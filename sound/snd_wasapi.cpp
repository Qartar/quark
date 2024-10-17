// snd_wasapi.cpp
//

#include "snd_wasapi.h"
#include "cm_shared.h"

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>

////////////////////////////////////////////////////////////////////////////////
/**
 * Due to the dubious conventions of Windows API design this entire class is
 * required to handle various callbacks from the audio system, or for our use-case
 * a single callback.
 */
class audio_notification_client : public IMMNotificationClient
{
public:
    audio_notification_client(audio_device_wasapi& device)
        : _device(device)
        , _refcount(1)
    {}
    virtual ~audio_notification_client() {}

    //
    // IUnknown methods
    //

    virtual ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&_refcount);
    }

    virtual ULONG STDMETHODCALLTYPE Release() override {
        ULONG refcount = InterlockedDecrement(&_refcount);
        if (!refcount) {
            delete this;
        }
        return refcount;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface) override {
        if (!ppvInterface) {
            return E_POINTER;
        } else if (riid == __uuidof(IUnknown)) {
            AddRef();
            *ppvInterface = (IUnknown*)this;
        } else if (riid == __uuidof(IMMNotificationClient)) {
            AddRef();
            *ppvInterface = (IMMNotificationClient*)this;
        } else {
            *ppvInterface = nullptr;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    //
    // IMMNotificationClient methods
    //

    virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR /*pwstrDeviceId*/, DWORD /*dwNewState*/) override {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR /*pwstrDeviceId*/) override {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR /*pwstrDeviceId*/) override {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole /*role*/, LPCWSTR /*pwstrDefaultDeviceId*/) override {
        if (flow == eRender) {
            _device.on_default_render_device_changed();
        }
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR /*pwstrDeviceId*/, const PROPERTYKEY /*key*/) override {
        return S_OK;
    }

protected:
    audio_device_wasapi& _device;
    ULONG _refcount;
};

//------------------------------------------------------------------------------
static char const* hresult_to_string(HRESULT hr)
{
#define CASE(x) case x: return #x
    switch (hr) {
        CASE(E_NOINTERFACE);
        CASE(E_POINTER);
        CASE(E_NOTFOUND);
        CASE(E_INVALIDARG);
        CASE(E_OUTOFMEMORY);
        CASE(AUDCLNT_E_NOT_INITIALIZED);
        CASE(AUDCLNT_E_DEVICE_INVALIDATED);
        CASE(AUDCLNT_E_NOT_STOPPED);
        CASE(AUDCLNT_E_BUFFER_TOO_LARGE);
        CASE(AUDCLNT_E_UNSUPPORTED_FORMAT);
        CASE(AUDCLNT_E_SERVICE_NOT_RUNNING);
        CASE(AUDCLNT_E_EVENTHANDLE_NOT_SET);
        default: return "<unrecognized error>";
    }
#undef CASE
}

//------------------------------------------------------------------------------
audio_device_wasapi::audio_device_wasapi(IMMDeviceEnumerator* device_enumerator)
    : _buffer_info{}
    , _device_enumerator(device_enumerator)
    , _notification_client(new audio_notification_client(*this))
    , _device(nullptr)
    , _audio_client(nullptr)
    , _audio_render_client(nullptr)
    , _render_format(nullptr)
    , _reacquire_required(false)
{
    assert(device_enumerator);
    _device_enumerator->RegisterEndpointNotificationCallback(_notification_client);
}

//------------------------------------------------------------------------------
audio_device_wasapi* audio_device_wasapi::create()
{
    IMMDeviceEnumerator* device_enumerator = nullptr;
    audio_device_wasapi* audio_device = nullptr;

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        NULL,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        (void**)&device_enumerator);

    if (FAILED(hr)) {
        log::message("failed to create device enumerator: %s\n", hresult_to_string(hr));
        return nullptr;
    }

    audio_device = new audio_device_wasapi(device_enumerator);
    if (!audio_device->acquire()) {
        delete audio_device;
        audio_device = nullptr;
    }

    return audio_device;
}

//------------------------------------------------------------------------------
void audio_device_wasapi::destroy()
{
    release();

    if (_device_enumerator) {
        if (_notification_client) {
            _device_enumerator->UnregisterEndpointNotificationCallback(_notification_client);
        }
        _device_enumerator->Release();
        _device_enumerator = nullptr;
    }

    if (_notification_client) {
        _notification_client->Release();
        _notification_client = nullptr;
    }
}

//------------------------------------------------------------------------------
bool audio_device_wasapi::acquire()
{
    assert(_device_enumerator);
    assert(!_device);
    assert(!_audio_client);
    assert(!_audio_render_client);

    //
    // Get and activate a suitable endpoint
    //

    _device = get_render_endpoint();
    if (!_device) {
        return false;
    }

    HRESULT hr = _device->Activate(
        __uuidof(IAudioClient),
        CLSCTX_ALL,
        NULL,
        (void**)&_audio_client);

    if (FAILED(hr)) {
        log::message("failed to activate client: %s\n", hresult_to_string(hr));
        return false;
    }

    //
    // Get a suitable rendering format
    //

    _render_format = get_render_format();
    if (!_render_format) {
        return false;
    }

    _buffer_info.channels = _render_format->nChannels;
    _buffer_info.bitwidth = _render_format->wBitsPerSample;
    _buffer_info.frequency = _render_format->nSamplesPerSec;

    log::message("output buffer format:\n");
    log::message("...channels:  %d\n", _render_format->nChannels);
    log::message("...bit width: %d\n", _render_format->wBitsPerSample);
    log::message("...frequency: %u\n", _render_format->nSamplesPerSec);
    if (_render_format->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        GUID subformat = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(_render_format)->SubFormat;

        if (subformat == KSDATAFORMAT_SUBTYPE_PCM) {
            log::message("...subformat: PCM\n");
        } else if (subformat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            log::message("...subformat: IEEE_FLOAT\n");
        }
    } else if (_render_format->wFormatTag == WAVE_FORMAT_PCM) {
        log::message("...subformat: PCM\n");
    } else if (_render_format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        log::message("...subformat: IEEE_FLOAT\n");
    }

    //
    // Initialize the audio client
    //

    REFERENCE_TIME hnsBufferDuration = 10000000 / 30;
    hr = _audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        hnsBufferDuration,
        0,
        _render_format,
        NULL);

    if (FAILED(hr)) {
        log::message("failed to initialize audio client: %s\n", hresult_to_string(hr));
        return false;
    }

    //
    // Get the rendering service and start rendering
    //

    hr = _audio_client->GetService(
        __uuidof(IAudioRenderClient),
        (void**)&_audio_render_client);

    if (FAILED(hr)) {
        log::message("failed to get audio render client: %s\n", hresult_to_string(hr));
        return false;
    }

    return start_rendering();
}

//------------------------------------------------------------------------------
bool audio_device_wasapi::reacquire()
{
    _reacquire_required = false;

    // If we were clever we might be able to optimize this process a little but..
    release();
    return acquire();
}

//------------------------------------------------------------------------------
void audio_device_wasapi::release()
{
    if (_audio_client) {
        _audio_client->Stop();
        _audio_client->Release();
        _audio_client = nullptr;
    }

    if (_audio_render_client) {
        _audio_render_client->Release();
        _audio_render_client = nullptr;
    }

    if (_render_format) {
        CoTaskMemFree(_render_format);
        _render_format = nullptr;
    }

    if (_device) {
        _device->Release();
        _device = nullptr;
    }
}

//------------------------------------------------------------------------------
IMMDevice* audio_device_wasapi::get_render_endpoint() const
{
    assert(_device_enumerator);

    IMMDevice* endpoint = nullptr;

    // Explicit audio device selection would go here. Just use the default endpoint for now.
    HRESULT hr = _device_enumerator->GetDefaultAudioEndpoint(
        eRender,
        eConsole,
        &endpoint);

    if (FAILED(hr)) {
        log::message("failed to get default audio endpoint: %s\n", hresult_to_string(hr));
        return nullptr;
    }

    IPropertyStore* props = nullptr;
    endpoint->OpenPropertyStore(STGM_READ, &props);

    if (props) {
        PROPVARIANT varname;
        PropVariantInit(&varname);

        props->GetValue(PKEY_Device_FriendlyName, &varname);
        if (varname.vt != VT_EMPTY) {
            log::message("Using default audio endpoint: ^fff%S^xxx\n", varname.pwszVal);
        }

        PropVariantClear(&varname);
        props->Release();
    }

    return endpoint;
}

//------------------------------------------------------------------------------
WAVEFORMATEX* audio_device_wasapi::get_render_format() const
{
    assert(_audio_client);

    WAVEFORMATEX* device_format = nullptr;
    HRESULT hr = _audio_client->GetMixFormat(&device_format);
    if (FAILED(hr)) {
        log::message("failed to get mix format: %s\n", hresult_to_string(hr));
        if (device_format) {
            CoTaskMemFree(device_format);
        }
        return nullptr;
    }

    // Compatible format selection would go here (e.g. select stereo format if
    // given a surround sound format)

    return device_format;
}

//------------------------------------------------------------------------------
bool audio_device_wasapi::start_rendering() const
{
    assert(_audio_client);
    assert(_audio_render_client);

    //
    // Clear rendering buffer
    //

    UINT32 buffer_frame_fount;
    HRESULT hr = _audio_client->GetBufferSize(&buffer_frame_fount);
    if (FAILED(hr)) {
        log::message("failed to get buffer size: %s\n", hresult_to_string(hr));
        return false;
    }

    BYTE* buffer_data = nullptr;
    hr = _audio_render_client->GetBuffer(buffer_frame_fount, &buffer_data);
    if (FAILED(hr)) {
        log::message("failed to get render buffer: %s\n", hresult_to_string(hr));
        return false;
    }

    hr = _audio_render_client->ReleaseBuffer(buffer_frame_fount, AUDCLNT_BUFFERFLAGS_SILENT);
    if (FAILED(hr)) {
        log::message("failed to clear render buffer: %s\n", hresult_to_string(hr));
        return false;
    }

    //
    // Start rendering
    //

    hr = _audio_client->Start();
    if (FAILED(hr)) {
        log::message("failed to start audio client: %s\n", hresult_to_string(hr));
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
buffer_info_t audio_device_wasapi::get_buffer_info()
{
    // This is a ~10-20 ms operation and should ultimately go somewhere else,
    // however given the current design of the sound interfaces this is the best
    // place as it's called immediately prior to writing sound data to the buffer.
    if (_reacquire_required) {
        reacquire();
    }

    buffer_info_t info = _buffer_info;
    UINT32 num_buffer_frames = 0, num_padding_frames = 0;
    HRESULT hr = _audio_client->GetBufferSize(&num_buffer_frames);
    if (FAILED(hr)) {
        log::message("failed get client buffer size: %s\n", hresult_to_string(hr));
    }

    hr = _audio_client->GetCurrentPadding(&num_padding_frames);
    if (FAILED(hr)) {
        log::message("failed get client buffer padding: %s\n", hresult_to_string(hr));
    }

    info.size = num_buffer_frames - num_padding_frames;
    return info;
}

//------------------------------------------------------------------------------
void audio_device_wasapi::write(byte* data, int num_bytes)
{
    int bytes_per_frame = _buffer_info.channels * _buffer_info.bitwidth / 8;
    BYTE* buffer_data = nullptr;

    HRESULT hr = _audio_render_client->GetBuffer(num_bytes / bytes_per_frame, &buffer_data);
    if (FAILED(hr)) {
        log::message("failed to get render client buffer: %s\n", hresult_to_string(hr));
    } else {
        // Sound mixing interface currently mixes to 16-bit PCM format in a 32-bit
        // container, convert to floating point while copying into the render buffer.
        int const* src = reinterpret_cast<int const*>(data);
        int const* end = reinterpret_cast<int const*>(data + num_bytes);
        float* dst = reinterpret_cast<float*>(buffer_data);
        while (src < end) {
            *dst++ = (*src++) * (1.f / 65535.f);
        }

        hr = _audio_render_client->ReleaseBuffer(num_bytes / bytes_per_frame, 0);
        if (FAILED(hr)) {
            log::message("failed to release render client buffer: %s\n", hresult_to_string(hr));
        }
    }
}

//------------------------------------------------------------------------------
void audio_device_wasapi::on_default_render_device_changed()
{
    // This notification is received asynchronously and multiple times for each
    // device switch (once for each "role", cf. eRole) so queue up a reacquire
    // instead of doing it here.
    _reacquire_required = true;
}
