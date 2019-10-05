// snd_device.cpp
//

#include "snd_main.h"
#include "snd_device.h"
#include "snd_dsound.h"

//------------------------------------------------------------------------------
cAudioDevice *cAudioDevice::create(HWND hwnd)
{
#if defined(_WIN32)
    cAudioDevice* device;
    device_state_t state = device_fail;

    log::message("------ initializing sound ------\n");

    //  try directsound
    if ((device = new cDirectSoundDevice(hwnd))) {
        if ((state = device->get_state()) == device_ready) {
            return device;
        }

        device->destroy();
        delete device;

        if (state == device_abort) {
            return NULL;
        }
    }
#endif // defined(_WIN32)

    return NULL;
}

//------------------------------------------------------------------------------
void cAudioDevice::destroy(cAudioDevice* device)
{
    log::message("------ shutting down sound ------\n");

    if (device) {
        device->destroy();
        delete device;
    }

    return;
}
