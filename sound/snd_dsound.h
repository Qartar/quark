// snd_dsound.h
//

#pragma once

#if defined(_WIN32)

#include <windows.h>
#include <dsound.h>

#include "cm_config.h"
#include "cm_sound.h"

typedef HRESULT iDirectSoundCreate (GUID *, LPDIRECTSOUND8 *, IUnknown *);

#define DEFAULT_BUFFER_SIZE 0x10000

//------------------------------------------------------------------------------
class cDirectSoundDevice : public cAudioDevice
{
public:
    cDirectSoundDevice(HWND hwnd);

    virtual void destroy() override;

    virtual device_state_t get_state() override { return _state; }
    virtual buffer_info_t get_buffer_info() override;
    virtual void write(byte *data, int num_bytes) override;

private:
    LPDIRECTSOUND8 _directsound;
    LPDIRECTSOUNDBUFFER _submix_buffer;
    LPDIRECTSOUNDBUFFER _primary_buffer;
    WAVEFORMATEX _buffer_format;
    DSBCAPS _buffer_caps;
    int _buffer_offset;

    device_state_t _state;

    config::boolean snd_primary;
    config::boolean snd_dsfocus;
    config::integer snd_frequency;

    HMODULE _hmodule;
    HWND _hwnd;

private:
    result create_buffers();
    void destroy_buffers();

    void mix_stereo16(samplepair_t* input, stereo16_t* output, int num_samples, int volume);
};

#endif // defined(_WIN32)
