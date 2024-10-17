// snd_wasapi.h
//

#pragma once

#include "snd_device.h"

#include "cm_config.h"
#include "cm_sound.h"

// mmeapi.h
typedef struct tWAVEFORMATEX WAVEFORMATEX;

// mmdeviceapi.h
struct IMMDeviceEnumerator;
struct IMMDevice;
struct IMMNotificationClient;

// Audioclient.h
struct IAudioClient;
struct IAudioRenderClient;

//------------------------------------------------------------------------------
class audio_device_wasapi : public cAudioDevice
{
public:
    static audio_device_wasapi* create();
    virtual void destroy() override;

    virtual device_state_t get_state() override { return device_ready; }
    virtual buffer_info_t get_buffer_info() override;
    virtual void write(byte *data, int num_bytes) override;

protected:
    buffer_info_t _buffer_info;

    IMMDeviceEnumerator* _device_enumerator;
    IMMNotificationClient* _notification_client;

    IMMDevice* _device;

    IAudioClient* _audio_client;
    IAudioRenderClient* _audio_render_client;

    WAVEFORMATEX* _render_format;

    bool _reacquire_required;

protected:
    audio_device_wasapi(IMMDeviceEnumerator* enumerator);

    //! Acquire an audio endpoint, initialize an audio client, and start rendering.
    bool acquire();
    //! Reacquire an audio endpoint in case of audio reset or change to default endpoint.
    bool reacquire();
    //! Stop rendering and release the client interfaces and endpoint.
    void release();

    //! Get a suitable rendering endpoint from the device enumerator.
    IMMDevice* get_render_endpoint() const;
    //! Get a suitable wave format for rendering.
    WAVEFORMATEX* get_render_format() const;
    //! Initialize the render buffer and start the rendering service.
    bool start_rendering() const;

    //
    // audio_notification_client callbacks
    //

    friend class audio_notification_client;
    void on_default_render_device_changed();
};
