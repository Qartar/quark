// snd_wav_source.cpp
//

#include "snd_main.h"
#include "snd_wav_source.h"
#include "snd_wav_cache.h"
#include "snd_wav_stream.h"
#include "snd_wav_resource.h"

#if defined(_WIN32)
#   define NOMINMAX
#   include <Windows.h>
#endif // defined(_WIN32)

//------------------------------------------------------------------------------
cSoundSource* cSoundSource::create(string::view filename)
{
    std::size_t filelen;

    cSoundSource* source = nullptr;

    if (strncmp(filename.begin() + filename.length() - 4, ".wav", 4) == 0) {
        filelen = file::open(filename, file::mode::read).size();

        if (filelen == 0) {
            source = (cSoundSource *)new cSoundWaveResource;
        } else if (filelen > STREAM_THRESHOLD) {
            source = (cSoundSource *)new cSoundWaveStream;
        } else {
            source = (cSoundSource *)new cSoundWaveCache;
        }
    } else {
        log::message("unknown sound format: %s\n", filename.c_str());
    }

    if (source && succeeded(source->load(filename))) {
        return source;
    } else {
        if (source) {
            delete source;
        }
        return nullptr;
    }
}

//------------------------------------------------------------------------------
bool cSoundWaveSource::parse_chunk(chunk_file &chunk)
{
    switch (chunk.id()) {
        case CHUNK_FMT:
            return parse_format(chunk);

        case CHUNK_CUE:
            return parse_cue(chunk);

        case CHUNK_DATA:
            return parse_data(chunk);

        default:
            return false;
    }
}

//------------------------------------------------------------------------------
bool cSoundWaveSource::parse_format(chunk_file &chunk)
{
#if defined(_WIN32)
    WAVEFORMATEX        wfx;

    chunk.read((byte *)&wfx, chunk.size());

    _format.format = wfx.wFormatTag;
    _format.channels = wfx.nChannels;
    _format.bitwidth = wfx.wBitsPerSample;
    _format.frequency = wfx.nSamplesPerSec;

    return true;
#else // !defined(_WIN32)
    return false;
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
bool cSoundWaveSource::parse_cue(chunk_file &chunk)
{
    struct cuePoint_s {
        int     cueID;
        int     cuePos;
        int     chunkID;
        int     chunkStart;
        int     blockStart;
        int     sampleOffset;
    } cue_point;

    /*int cue_count =*/ chunk.read_int();

    chunk.read((byte *)&cue_point, sizeof(cue_point));
    _loop_start = cue_point.sampleOffset;

    // dont care about the rest
    return true;
}

//------------------------------------------------------------------------------
float cSoundWaveSource::get_position(float position) const
{
    return std::fmod(position, static_cast<float>(_num_samples));
}
