set(SHARED_SOURCES
    cm_bounds.h
    cm_color.h
    cm_config.cpp
    cm_config.h
    cm_console.cpp
    cm_console.h
    cm_error.h
    cm_filesystem.cpp
    cm_filesystem.h
    cm_geometry.h
    cm_keys.h
    cm_matrix.h
    cm_parser.cpp
    cm_parser.h
    cm_random.h
    cm_shared.cpp
    cm_shared.h
    cm_sound.h
    cm_string.cpp
    cm_string.h
    cm_time.h
    cm_vector.h

    shared.natvis
)

add_library(shared STATIC ${SHARED_SOURCES})
target_include_directories(shared PUBLIC .)
source_group("\\" FILES ${SHARED_SOURCES})
