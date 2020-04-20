// snd_device.h
//

#pragma once

#include "cm_sound.h"

//------------------------------------------------------------------------------
typedef struct buffer_info_s
{
    int     channels;
    int     bitwidth;
    int     frequency;

    int     read, write;
    int     size;
} buffer_info_t;

//------------------------------------------------------------------------------
typedef enum device_state_s
{
    device_ready,
    device_fail,
    device_abort
} device_state_t;

//------------------------------------------------------------------------------
class cAudioDevice
{
public:
    static cAudioDevice *create (HWND hwnd);
    static void destroy(cAudioDevice* device);

    virtual ~cAudioDevice() {}
    virtual void destroy() = 0;

    virtual device_state_t get_state() = 0;
    virtual buffer_info_t get_buffer_info() = 0;

    virtual void write(byte* data, int size) = 0;
};
