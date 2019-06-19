// snd_files.h
//

#pragma once

#include "cm_sound.h"

//------------------------------------------------------------------------------
struct sound_format {
    int     format;
    int     channels;
    int     bitwidth;
    int     frequency;
};

//------------------------------------------------------------------------------
class cSoundSource
{
public:
    virtual ~cSoundSource() {};

    static cSoundSource* create(string::view filename);

    virtual std::size_t get_samples(byte* samples, int num_samples, int sample_offset, bool looping) = 0;
    virtual sound_format const* get_format() const = 0;
    virtual string::view get_filename() const = 0;
    virtual float get_position(float position) const = 0;

private:
    virtual result load(string::view filename) = 0;
    virtual void free() = 0;
};
