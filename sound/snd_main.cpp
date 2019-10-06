// snd_main.cpp
//

#include "snd_main.h"

////////////////////////////////////////////////////////////////////////////////
sound::system* pSound = nullptr;

//------------------------------------------------------------------------------
result sound::system::create()
{
    pSound = new cSound;
    return result::success;
}

//------------------------------------------------------------------------------
void sound::system::destroy()
{
    delete pSound;
    pSound = nullptr;
}

//------------------------------------------------------------------------------
cSound::cSound()
    : snd_disable("snd_disable", false, config::archive, "disables sound playback")
    , snd_volume("snd_volume", 0.5f, config::archive, "sound volume")
    , snd_frequency("snd_frequency", 22050, config::archive, "sound playback speed")
    , snd_mixahead("snd_mixahead", 0.1f, config::archive, "sound mix ahead time, in seconds")
    , snd_primary("snd_primary", false, config::archive, "use primary sound buffer")
    , _initialized(false)
    , _audio_device(nullptr)
    , _paint_buffer{}
    , _channel_buffer{}
{
}

//------------------------------------------------------------------------------
result cSound::on_create(HWND hwnd)
{
    if (snd_disable) {
        return result::success;
    }

    _audio_device = cAudioDevice::create(hwnd);
    if (!_audio_device || _audio_device->get_state() != device_ready) {
        return result::failure;
    } else {
        return result::success;
    }
}

//------------------------------------------------------------------------------
void cSound::on_destroy()
{
    if (_paint_buffer.data) {
        delete [] _paint_buffer.data;
        delete [] _channel_buffer.data;

        _paint_buffer.data = NULL;
        _paint_buffer.size = 0;

        _channel_buffer.data = NULL;
        _channel_buffer.size = 0;
    }

    cAudioDevice::destroy(_audio_device);
    _audio_device = NULL;
}

//------------------------------------------------------------------------------
void cSound::update()
{
    if (!_audio_device) {
        return;
    }

    buffer_info_t info = _audio_device->get_buffer_info();
    int num_samples = static_cast<int>(snd_mixahead * info.frequency);
    paintbuffer_t* buffer = get_paint_buffer(num_samples * info.channels * PAINTBUFFER_BYTES);

    buffer->frequency = info.frequency;
    buffer->channels = info.channels;
    buffer->volume = static_cast<int>(snd_volume * 255.f);

    int num_written = info.write - info.read;
    if (num_written < 0) {
        num_written += info.size;
    }

    if (num_samples > info.size) {
        num_samples = info.size;
    }

    num_samples -= num_written / (info.channels * info.bitwidth / 8);

    if (num_samples < 0) {
        return;
    }

    mix_channels(buffer, num_samples);

    int num_bytes = num_samples * info.channels * info.bitwidth / 8;

    _audio_device->write(buffer->data, num_bytes);

    // free one-shot channels
    for (std::size_t ii = 0; ii < _channels.size(); ++ii) {
        while (ii < _channels.size()
               && !_channels[ii]->playing()
               && !_channels[ii]->is_reserved()) {
            free_channel_index(ii);
        }
    }
}

//------------------------------------------------------------------------------
paintbuffer_t* cSound::get_paint_buffer(int num_bytes)
{
    if (!_paint_buffer.data) {
        _paint_buffer.data = new byte[num_bytes];
        _paint_buffer.size = num_bytes;

        _channel_buffer.data = new byte[num_bytes];
        _channel_buffer.size = num_bytes;
    } else if (num_bytes != _paint_buffer.size) {
        delete [] _paint_buffer.data;
        delete [] _channel_buffer.data;

        _paint_buffer.data = new byte[num_bytes];
        _paint_buffer.size = num_bytes;

        _channel_buffer.data = new byte[num_bytes];
        _channel_buffer.size = num_bytes;
    }

    memset(_paint_buffer.data, 0, _paint_buffer.size);
    return &_paint_buffer;
}

//------------------------------------------------------------------------------
void cSound::set_listener(vec3 origin, vec3 forward, vec3 right, vec3 up)
{
    _origin = origin;
    _axis = mat3(forward, right, up);
}

//------------------------------------------------------------------------------
sound::asset cSound::load_sound(string::view filename)
{
#if defined(_WIN32)
    auto it = _sounds_by_name.find(filename);
    if (it != _sounds_by_name.cend()) {
        return it->second;
    }

    cSoundSource* sound = cSoundSource::create(filename);
    if (sound) {
        auto asset = static_cast<sound::asset>(_sounds.size() + 1);
        _sounds.emplace_back(sound);
        _sounds_by_name[string::buffer(filename)] = asset;
        return asset;
    } else {
        return sound::asset::invalid;
    }
#else
    return sound::asset::invalid;
#endif // defined(_WIN32)
}

//------------------------------------------------------------------------------
cSoundSource* cSound::get_sound(sound::asset asset)
{
    std::size_t index = static_cast<std::size_t>(asset) - 1;
    if (index < _sounds.size()) {
        return _sounds[index].get();
    } else {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
result cSound::play(sound::asset asset, vec3 origin, float volume, float attenuation)
{
    sound::channel* ch = alloc_channel(false);

    if (ch) {
        ch->set_volume(volume);
        ch->set_origin(origin);
        ch->set_frequency(1.0f);
        ch->set_attenuation(attenuation);

        return ch->play(asset);
    } else {
        return result::failure;
    }
}

//------------------------------------------------------------------------------
sound::channel* cSound::allocate_channel()
{
    if (!_audio_device) {
        return nullptr;
    } else {
        return alloc_channel(true);
    }
}

//------------------------------------------------------------------------------
cSoundChannel *cSound::alloc_channel(bool reserve)
{
    if (_free_channels.size()) {
        _channels.push_back(std::move(_free_channels.back()));
        _free_channels.pop_back();
    } else {
        _channels.push_back(std::make_unique<cSoundChannel>(this));
    }
    _channels.back()->set_reserved(reserve);
    return _channels.back().get();
}

//------------------------------------------------------------------------------
void cSound::free_channel(sound::channel* chan)
{
    auto it = std::find_if(_channels.begin(), _channels.end(),
        [chan](std::unique_ptr<cSoundChannel> const& elem) {
            return elem.get() == chan;
        });
    free_channel_index(std::distance(_channels.begin(), it));
}

//------------------------------------------------------------------------------
void cSound::free_channel_index(std::size_t index)
{
    assert(index < _channels.size());
    _channels[index]->stop();
    _free_channels.push_back(std::move(_channels[index]));
    _channels.erase(_channels.begin() + index);
}
