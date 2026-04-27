// snd_channel.cpp
//

#include "snd_main.h"

//------------------------------------------------------------------------------
void cSound::mix_channels(paintbuffer_t* buffer, int num_samples)
{
    for (auto& channel : _channels) {
        if (channel->playing()) {
            channel->mix(buffer, num_samples);
        }
    }
}

//------------------------------------------------------------------------------
result cSoundChannel::play(sound::asset asset, bool looping)
{
    if (asset == sound::asset::invalid) {
        return result::failure;
    }

    _source = _sound->get_sound(asset);

    if (!_source) {
        log::warning("could not play sound %z: does not exist\n", asset);
        return result::failure;
    }

    _sample_pos = 0;
    _is_playing = true;
    _is_looping = looping;

    return result::success;
}

//------------------------------------------------------------------------------
result cSoundChannel::play(sound::asset asset)
{
    return play(asset, false);
}

//------------------------------------------------------------------------------
result cSoundChannel::loop(sound::asset asset)
{
    return play(asset, true);
}

//------------------------------------------------------------------------------
void cSoundChannel::stop()
{
    _is_playing = false;
    _is_looping = false;
}

//------------------------------------------------------------------------------
void cSoundChannel::mix(paintbuffer_t* buffer, int samples)
{
    int     volume = (int)(_volume * buffer->volume * 255) >> 8;

    sound_format const* format = _source->get_format();

    float   rate = _frequency * (float)format->frequency / (float)buffer->frequency;

    if (format->channels == 1 && format->bitwidth == 16) {
        _mix_mono16(buffer->data, rate, volume, samples);
    }
    if (format->channels == 2 && format->bitwidth == 16) {
        _mix_stereo16(buffer->data, rate, volume, samples);
    }
}

//------------------------------------------------------------------------------
#define SAMPLE(a,b,c)   ( (float)(a[b]*(1-c))+(float)(a[b+1]*(c)) )

void cSoundChannel::_mix_mono16(void* buffer, float rate, int volume, int num_samples)
{
    samplepair_t* output = (samplepair_t *)buffer;
    short* input = (short *)_sound->get_channel_buffer()->data;

    int spatial_vol[2];

    _spatialize_stereo(volume, spatial_vol);

    int actual_samples = static_cast<int>(num_samples * rate);
    int actual_offset = static_cast<int>(_sample_pos);

    std::size_t samples_read = _source->get_samples((byte *)input, actual_samples, actual_offset, _is_looping);
    float sample_pos = _sample_pos - floor(_sample_pos);

    for (int ii = 0; sample_pos < samples_read; ++ii) {
        int sample_index = static_cast<int>(sample_pos);
        float sample_frac = sample_pos - std::trunc(sample_pos);

        output[ii].left += static_cast<int>(spatial_vol[0] * SAMPLE(input, sample_index, sample_frac)) >> 8;
        output[ii].right += static_cast<int>(spatial_vol[1] * SAMPLE(input, sample_index, sample_frac)) >> 8;

        sample_pos += rate;
    }

    if (samples_read < (int)num_samples * rate) {
        _is_playing = false;
    }

    _sample_pos = _source->get_position(_sample_pos + rate * samples_read);
}

#undef SAMPLE

//------------------------------------------------------------------------------
#define SAMPLE(a,b,c,d) ( (float)(a[c].b*(1-d))+(float)(a[c+1].b*(d)) )

void cSoundChannel::_mix_stereo16(void* buffer, float rate, int volume, int num_samples)
{
    samplepair_t* output = (samplepair_t *)buffer;
    stereo16_t* input = (stereo16_t *)_sound->get_channel_buffer()->data;

    int spatial_vol[2];

    _spatialize_stereo(volume, spatial_vol);

    int actual_samples = static_cast<int>(num_samples * rate);
    int actual_offset = static_cast<int>(_sample_pos);

    std::size_t samples_read = _source->get_samples((byte *)input, actual_samples, actual_offset, _is_looping);
    float sample_pos = _sample_pos - floor(_sample_pos);

    for (int ii = 0; sample_pos < samples_read; ++ii) {
        int sample_index = static_cast<int>(sample_pos);
        float sample_frac = sample_pos - std::trunc(sample_pos);

        output[ii].left += static_cast<int>(spatial_vol[0] * SAMPLE(input, left, sample_index, sample_frac)) >> 8;
        output[ii].right += static_cast<int>(spatial_vol[1] * SAMPLE(input, right, sample_index, sample_frac)) >> 8;

        sample_pos += rate;
    }

    if (samples_read < (int)num_samples * rate) {
        _is_playing = false;
    }

    _sample_pos = _source->get_position(_sample_pos + rate * samples_read);
}

#undef SAMPLE

//------------------------------------------------------------------------------
#define ATTN_LEN        1000.0

void cSoundChannel::_spatialize_mono(int in, int *out)
{
    if (_attenuation == ATTN_STATIC) {
        out[0] = in;
    } else {
        double dist = (_origin - _sound->_origin).length();
        double attn = clamp(pow(ATTN_LEN / dist, _attenuation), 0.0, 1.0);

        out[0] = static_cast<int>(in * attn);
    }
}

//------------------------------------------------------------------------------
void cSoundChannel::_spatialize_stereo(int in, int out[2])
{
    vec3 dir = _origin - _sound->_origin;
    double dist = dir.normalize_length();
    double dp = dir.dot(_sound->_axis[1]);

    if (_attenuation == ATTN_STATIC) {
        out[0] = static_cast<int>(in * 0.5 * (1.0 - dp));
        out[1] = static_cast<int>(in * 0.5 * (1.0 + dp));
    } else {
        double attn = clamp(pow(ATTN_LEN / dist, _attenuation), 0.0, 1.0);

        out[0] = static_cast<int>(in * 0.5 * (1.0 - dp) * attn);
        out[1] = static_cast<int>(in * 0.5 * (1.0 + dp) * attn);
    }
}
