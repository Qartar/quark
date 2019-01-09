//  cm_sound.h
//

#pragma once

#include "cm_error.h"
#include "cm_string.h"
#include "cm_vector.h"

#ifndef _WINDOWS_
typedef struct HWND__ *HWND;
#endif // _WINDOWS_

////////////////////////////////////////////////////////////////////////////////
namespace sound {

enum class asset
{
    invalid = 0
};

//------------------------------------------------------------------------------
class channel
{
public:
    virtual bool playing() const = 0;
    virtual bool looping() const = 0;

    virtual result play(sound::asset asset) = 0;
    virtual result loop(sound::asset asset) = 0;
    virtual void stop() = 0;

    virtual void set_origin(vec3 origin) = 0;
    virtual void set_volume(float volume) = 0;
    virtual void set_frequency(float frequency) = 0;
    virtual void set_attenuation(float attenuation) = 0;
};

//------------------------------------------------------------------------------
class system
{
public:
    static result create ();
    static void destroy ();

    virtual ~system() {}

    virtual result on_create(HWND hwnd) = 0;
    virtual void on_destroy() = 0;

    virtual void update() = 0;

    virtual void set_listener(vec3 origin, vec3 forward, vec3 right, vec3 up) = 0;

    virtual result play(sound::asset asset, vec3 origin, float volume, float attenuation) = 0;

    virtual sound::channel* allocate_channel() = 0;
    virtual void free_channel(sound::channel* channel) = 0;

    virtual sound::asset load_sound (string::view filename) = 0;
};

} // namespace sound

extern sound::system* pSound;
