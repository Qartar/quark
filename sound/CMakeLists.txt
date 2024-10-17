set(SOUND_SOURCES
    snd_channel.cpp
    snd_device.cpp
    snd_device.h
    snd_dsound.cpp
    snd_dsound.h
    snd_files.h
    snd_main.cpp
    snd_main.h
    snd_wasapi.cpp
    snd_wasapi.h
    snd_wav_cache.cpp
    snd_wav_cache.h
    snd_wav_reader.cpp
    snd_wav_source.cpp
    snd_wav_source.h
    snd_wav_stream.cpp
    snd_wav_stream.h
    snd_wav_resource.cpp
    snd_wav_resource.h
)

add_library(sound STATIC ${SOUND_SOURCES})
target_link_libraries(sound PUBLIC shared)
source_group("\\" FILES ${SOUND_SOURCES})
