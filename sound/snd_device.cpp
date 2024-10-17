// snd_device.cpp
//

#include "snd_main.h"
#include "snd_device.h"
#include "snd_dsound.h"
#include "snd_wasapi.h"

//------------------------------------------------------------------------------
cAudioDevice *cAudioDevice::create(HWND hwnd)
{
    cAudioDevice* device = nullptr;

    log::message("------ initializing sound ------\n");

    //  try WASAPI
    if (config::boolean("snd_wasapi", true, config::archive, "use Windows Audio Session API (WASAPI) for sound rendering")) {
        device = audio_device_wasapi::create();
    }

    //  try directsound
    if (!device && (device = new cDirectSoundDevice(hwnd))) {
        if (device->get_state() == device_ready) {
            return device;
        }
    
        device->destroy();
        delete device;
        device = nullptr;
    }

    return device;
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
