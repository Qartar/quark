// snd_dsound.cpp
//

#include "snd_main.h"
#include "snd_device.h"
#include "snd_dsound.h"

#if defined(_WIN32)

////////////////////////////////////////////////////////////////////////////////
HRESULT (WINAPI *pDirectSoundCreate)(GUID *, LPDIRECTSOUND8 *, IUnknown *);

//------------------------------------------------------------------------------
cDirectSoundDevice::cDirectSoundDevice(HWND hwnd)
    : snd_primary("snd_primary", false, config::archive, "use primary sound buffer")
    , snd_dsfocus("snd_dsfocus", true, config::archive, "")
    , snd_frequency("snd_frequency", 22050, config::archive, "sound playback speed")
{
    HRESULT         hResult;

    log::message("attempting to use DirectSound...\n");

    _directsound = NULL;
    _submix_buffer = NULL;
    _hmodule = NULL;

    _hwnd = hwnd;

    _state = device_fail;

    log::message("...linking dsound.dll: ");

    if ((_hmodule = LoadLibraryA("dsound.dll")) == NULL) {
        log::message("failed\n");
        return;
    } else {
        log::message("ok\n");
    }

    if ((pDirectSoundCreate = (HRESULT (__stdcall *)(GUID *, LPDIRECTSOUND8 *, IUnknown *))GetProcAddress(_hmodule, "DirectSoundCreate8")) == NULL) {
        log::message("...GetProcAddress failed\n");
        return;
    }

    log::message("...creating DirectSound object: ");
    if ((hResult = pDirectSoundCreate(NULL, &_directsound, NULL)) != DS_OK) {
        if (hResult == DSERR_ALLOCATED) {
            log::message("failed, device in use\n");
            _state = device_abort;
        } else {
            log::message("failed\n");
        }

        return;
    } else {
        log::message("ok\n");
    }

    DSCAPS device_caps{};
    device_caps.dwSize = sizeof(DSCAPS);
    if (_directsound->GetCaps(&device_caps) != DS_OK) {
        log::message("...GetCaps failed\n");
        return;
    }

    if (device_caps.dwFlags & DSCAPS_EMULDRIVER) {
        log::message("...sound drivers not present\n");
        return;
    }

    if (failed(create_buffers())) {
        return;
    }

    log::message("...completed successfully\n");
    _state = device_ready;
}

//------------------------------------------------------------------------------
void cDirectSoundDevice::destroy()
{
    log::message("shutting down DirectSound...\n");

    if (_submix_buffer) {
        destroy_buffers();
    }

    if (_directsound) {
        log::message("...releasing DirectSound object\n");

        _directsound->Release();
        _directsound = NULL;
    }

    if (_hmodule) {
        log::message("...releasing dsound.dll\n");

        FreeLibrary(_hmodule);
        _hmodule = NULL;
    }
}

//------------------------------------------------------------------------------
result cDirectSoundDevice::create_buffers()
{
    DSBUFFERDESC dsbd;
    WAVEFORMATEX wfx;
    bool primary_set = false;

    log::message("creating DirectSound buffers...\n");

    log::message("...setting coop level to exclusive: ");
    if (_directsound->SetCooperativeLevel(_hwnd, DSSCL_EXCLUSIVE) != DS_OK) {
        log::message("failed\n");
        return result::failure;
    } else {
        log::message("ok\n");
    }

    memset(&wfx, 0, sizeof(wfx));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.wBitsPerSample = 16;
    wfx.nSamplesPerSec = clamp<int>(snd_frequency, 11025, 44100);
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    memset(&dsbd, 0, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat = 0;

    memset(&_buffer_caps, 0, sizeof(DSBCAPS));
    _buffer_caps.dwSize = sizeof(DSBCAPS);

    log::message("...creating primary buffer: ");
    if (_directsound->CreateSoundBuffer(&dsbd, &_primary_buffer, 0) == DS_OK) {
        log::message("ok\n");

        _buffer_format = wfx;
        if (_primary_buffer->SetFormat(&_buffer_format) != DS_OK) {
            log::message("...setting primary format: failed\n");
        } else {
            log::message("...setting primary format: ok\n");
            primary_set = true;
        }
    } else {
        log::message("failed\n");
    }

    if (!primary_set || !snd_primary) {
        _buffer_format = wfx;

        memset(&dsbd, 0, sizeof(DSBUFFERDESC));
        dsbd.dwSize = sizeof(DSBUFFERDESC);
        dsbd.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
        dsbd.dwBufferBytes = DEFAULT_BUFFER_SIZE;
        dsbd.lpwfxFormat = &_buffer_format;

        if (snd_dsfocus) {
            dsbd.dwFlags |= DSBCAPS_STICKYFOCUS;
        }

        log::message("...creating submix buffer: ");
        if (_directsound->CreateSoundBuffer(&dsbd, &_submix_buffer, 0) != DS_OK) {
            log::message("failed\n");
            return result::failure;
        } else {
            log::message("ok\n");
        }

        if (_submix_buffer->GetCaps(&_buffer_caps) != DS_OK) {
            log::message("...GetCaps failed\n");
            return result::failure;
        }

        log::message("...using submix buffer\n");
    } else {
        log::message("...setting coop level to writeprimary: ");
        if (_directsound->SetCooperativeLevel(_hwnd, DSSCL_WRITEPRIMARY) != DS_OK) {
            log::message("failed\n");
            return result::failure;
        } else {
            log::message("ok\n");
        }

        if (_primary_buffer->GetCaps(&_buffer_caps) != DS_OK) {
            log::message("...GetCaps failed\n");
            return result::failure;
        }

        _submix_buffer = _primary_buffer;
        log::message("...using primary buffer\n");
    }

    log::message("output buffer format:\n");
    log::message("...channels:  %d\n", _buffer_format.nChannels);
    log::message("...bit width: %d\n", _buffer_format.wBitsPerSample);
    log::message("...frequency: %d\n", _buffer_format.nSamplesPerSec);

    _submix_buffer->Play(0, 0, DSBPLAY_LOOPING);
    _submix_buffer->GetCurrentPosition(NULL, (LPDWORD)&_buffer_offset);

    return result::success;
}

//------------------------------------------------------------------------------
void cDirectSoundDevice::destroy_buffers()
{
    log::message("destroying DirectSound buffers...\n");

    if (_directsound) {
        log::message("...setting coop level to normal\n");
        _directsound->SetCooperativeLevel(_hwnd, DSSCL_NORMAL);
    }

    if (_submix_buffer && _submix_buffer != _primary_buffer) {
        log::message("...releasing secondary buffer\n");
        _submix_buffer->Stop();
        _submix_buffer->Release();

        if (_primary_buffer) {
            log::message("...releasing primary buffer\n");
            _primary_buffer->Release();
        }

        _primary_buffer = NULL;
        _submix_buffer = NULL;
    } else if (_submix_buffer) {
        log::message("...releasing primary buffer\n");

        _submix_buffer->Stop();
        _submix_buffer->Release();
        _primary_buffer = NULL;
        _submix_buffer = NULL;
    }
}

//------------------------------------------------------------------------------
buffer_info_t cDirectSoundDevice::get_buffer_info()
{
    buffer_info_t info;

    info.channels = _buffer_format.nChannels;
    info.bitwidth = _buffer_format.wBitsPerSample;
    info.frequency = _buffer_format.nSamplesPerSec;
    info.size = _buffer_caps.dwBufferBytes;

    _submix_buffer->GetCurrentPosition((LPDWORD)&info.read, (LPDWORD)&info.write);

    info.write = _buffer_offset;

    return info;
}

//------------------------------------------------------------------------------
void cDirectSoundDevice::write(byte* data, int num_bytes)
{
    LPVOID buffer[2];
    DWORD bytes[2], samples[2], status;

    _submix_buffer->GetStatus(&status);
    if (status & DSBSTATUS_BUFFERLOST) {
        _submix_buffer->Restore();
    }
    if (!(status & DSBSTATUS_PLAYING)) {
        _submix_buffer->Play(0, 0, DSBPLAY_LOOPING);
    }

    _submix_buffer->Lock(_buffer_offset, num_bytes, &buffer[0], &bytes[0], &buffer[1], &bytes[1], 0);

    samples[0] = bytes[0] / (_buffer_format.nChannels * _buffer_format.wBitsPerSample / 8);
    samples[1] = bytes[1] / (_buffer_format.nChannels * _buffer_format.wBitsPerSample / 8);

    mix_stereo16((samplepair_t *)data, (stereo16_t *)buffer[0], samples[0], 255);
    mix_stereo16((samplepair_t *)(data + bytes[0] * 2), (stereo16_t *)buffer[1], samples[1], 255);

    _submix_buffer->Unlock(buffer[0], bytes[0], buffer[1], bytes[1]);

    _buffer_offset = (_buffer_offset + bytes[0] + bytes[1]) % _buffer_caps.dwBufferBytes;
}

//------------------------------------------------------------------------------
void cDirectSoundDevice::mix_stereo16(samplepair_t* input, stereo16_t* output, int num_samples, int volume)
{
    for (int ii = 0; ii < num_samples; ++ii) {
        int left = (input[ii].left * volume) >> 8;
        output[ii].left = clamp<short>(left, INT16_MIN, INT16_MAX);

        int right = (input[ii].right * volume) >> 8;
        output[ii].right = clamp<short>(right, INT16_MIN, INT16_MAX);
    }
}

#endif // defined(_WIN32)
