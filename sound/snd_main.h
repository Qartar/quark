// snd_main.h
//

#pragma once

#define NOMINMAX
#include <windows.h>

#include "cm_shared.h"
#include "cm_config.h"
#include "cm_sound.h"

#include "snd_device.h"
#include "snd_files.h"

#include <memory>
#include <vector>

#define ATTN_STATIC     0.0f

class cSound;

//------------------------------------------------------------------------------
typedef struct samplepair_s {
    int  left;
    int  right;
} samplepair_t;

//------------------------------------------------------------------------------
typedef struct stereo16_s {
    short left;
    short right;
} stereo16_t;

//------------------------------------------------------------------------------
typedef struct stereo8_s {
    byte left;
    byte right;
} stereo8_t;

#define PAINTBUFFER_BYTES   4

//------------------------------------------------------------------------------
typedef struct paintbuffer_s {
    int frequency;
    int channels;
    int volume;
    int size;
    byte* data;
} paintbuffer_t;

//------------------------------------------------------------------------------
class cSoundChannel : public sound::channel
{
public:
    cSoundChannel(cSound* sound)
        : _sound(sound)
        , _source(nullptr)
        , _origin(vec3_zero)
        , _is_playing(false)
        , _is_looping(false)
        , _attenuation(0)
        , _frequency(1)
        , _volume(1)
        , _sample_pos(0)
        , _is_reserved(false)
    {}

    virtual bool playing() const override { return _is_playing; }
    virtual bool looping() const override { return _is_looping; }

    virtual result play(sound::asset asset, bool looping);
    virtual result play(sound::asset asset) override;
    virtual result loop(sound::asset asset) override;

    virtual void stop() override;

    virtual void set_origin(vec3 origin) override { assert(!isnan(origin)); _origin = origin; }
    virtual void set_volume(float volume) override { _volume = volume; }
    virtual void set_frequency(float frequency) override { _frequency = frequency; }
    virtual void set_attenuation(float attenuation) override { _attenuation = attenuation; }

    void set_reserved(bool b) { _is_reserved = b; }
    bool is_reserved() const { return _is_reserved; }

    void mix(paintbuffer_t* buffer, int samples);

private:
    cSound* _sound;
    cSoundSource* _source;

    vec3 _origin;
    
    bool _is_playing;
    bool _is_looping;
    float _attenuation;
    float _frequency;
    float _volume;

    float _sample_pos;
    bool _is_reserved;

    void _mix_mono16(void* buffer, float rate, int volume, int num_samples);
    void _mix_stereo16(void* buffer, float rate, int volume, int num_samples);

    void _spatialize_mono(int in, int *out);
    void _spatialize_stereo(int in, int out[2]);
};

//------------------------------------------------------------------------------
class cSound : public sound::system
{
public:
    cSound();
    ~cSound() {}

    virtual result on_create(HWND hWnd) override;
    virtual void on_destroy() override;

    virtual void update() override;

    virtual void set_listener(vec3 origin, vec3 forward, vec3 right, vec3 up) override;

    virtual result play(sound::asset asset, vec3 origin, float volume, float attenuation) override;

    virtual sound::channel* allocate_channel() override;
    virtual void free_channel(sound::channel* channel) override;

    //  registration

    sound::asset load_sound(string::view filename);
    cSoundSource* get_sound(sound::asset asset);

    //  mixing

    paintbuffer_t* get_channel_buffer() { return &_channel_buffer; }

    //  spatialization

    vec3 _origin;
    mat3 _axis;

private:
    config::boolean snd_disable;
    config::scalar snd_volume;
    config::integer snd_frequency;
    config::scalar snd_mixahead;
    config::boolean snd_primary;

    bool _initialized;

    cAudioDevice* _audio_device;

    paintbuffer_t* get_paint_buffer(int num_bytes);
    paintbuffer_t _paint_buffer;
    paintbuffer_t _channel_buffer;

    //
    //  sounds
    //

    std::vector<std::unique_ptr<cSoundSource>> _sounds;
    std::map<string::buffer, sound::asset> _sounds_by_name;

    //
    //  channels
    //

    cSoundChannel* alloc_channel(bool reserve);
    void free_channel_index(std::size_t index);

    void mix_channels(paintbuffer_t* buffer, int num_samples);
    std::vector<std::unique_ptr<cSoundChannel>> _channels;
    std::vector<std::unique_ptr<cSoundChannel>> _free_channels;
};
