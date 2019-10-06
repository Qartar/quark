cmake_minimum_required(VERSION 2.8.11)

project(quark)

# Configure version file
find_package(git)

set(SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake")
set(VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/version.h")

if(APPLE)
    add_compile_options(-DGL_SILENCE_DEPRECATION)
    INCLUDE_DIRECTORIES ( /System/Library/Frameworks )
    FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
    MARK_AS_ADVANCED (
            OpenGL_LIBRARY)
    SET(EXTRA_LIBS ${OpenGL_LIBRARY})
    link_libraries(${EXTRA_LIBS})
endif(APPLE)

if(GIT_FOUND)
    add_custom_target(quark-version ALL
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_NAME=${PROJECT_NAME}
            -DGIT_EXECUTABLE="${GIT_EXECUTABLE}"
            -DCONFIG_FILE="${SCRIPTS_DIR}/version.h.in"
            -DOUTPUT_FILE="${VERSION_FILE}"
            -P "${SCRIPTS_DIR}/version.cmake"
        SOURCES
            ${SCRIPTS_DIR}/version.cmake
            ${SCRIPTS_DIR}/version.h.in
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    configure_file(${SCRIPTS_DIR}/version.h.in ${VERSION_FILE})
endif()

if(MSVC)
    # Remove bad default compiler flags for Visual Studio
    string(REPLACE "/DWIN32 /D_WINDOWS /W3 /GR /EHsc" "/EHsc" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

    add_compile_options(
        /W4             # Enable warning level 4
        /WX             # Enable warnings as errors
        /wd4706         # Disable C4706: assignment within conditional expression
        /std:c++17      # Enable C++17 language features
        /GF             # Enable string pooling
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (NOT MSVC) # i.e. not clang-cl
        add_compile_options(
            "-Wall"
            "-Werror"
            "-std=c++17"
        )
    endif()
    add_compile_options(
        "-Wno-unknown-pragmas"
        "-Wno-missing-braces"
        "-Wno-missing-field-initializers"
    )
endif()

add_subdirectory(shared)
add_subdirectory(sound)
add_subdirectory(physics)
add_subdirectory(network)

set(QUARK_SOURCES
    game/g_aicontroller.cpp
    game/g_aicontroller.h
    game/g_button.cpp
    game/g_character.cpp
    game/g_character.h
    game/g_client.cpp
    game/g_handle.h
    game/g_handle.natvis
    game/g_menu.cpp
    game/g_menu.h
    game/g_network.cpp
    game/g_object.cpp
    game/g_object.h
    game/g_player.cpp
    game/g_player.h
    game/g_particles.cpp
    game/g_projectile.cpp
    game/g_projectile.h
    game/g_server.cpp
    game/g_session.cpp
    game/g_session.h
    game/g_shield.cpp
    game/g_shield.h
    game/g_ship.cpp
    game/g_ship.h
    game/g_subsystem.cpp
    game/g_subsystem.h
    game/g_usercmd.cpp
    game/g_usercmd.h
    game/g_weapon.cpp
    game/g_weapon.h
    game/g_world.cpp
    game/g_world.h
    render/r_draw.cpp
    render/r_font.cpp
    render/r_image.cpp
    render/r_main.cpp
    render/r_main.h
    render/r_model.cpp
    render/r_model.h
    render/r_particle.h
    render/r_window.cpp
    render/r_window.h
    system/resource.h
    system/win_main.cpp
    system/win_main.h
    system/win_include.h
    system/win_types.h
)

if (MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(WIN32_SOURCES system/resource.rc)
endif()

add_executable(quark WIN32
    ${QUARK_SOURCES}
    ${WIN32_SOURCES}
    ${VERSION_FILE}

    precompiled.cpp
    precompiled.h
)

add_dependencies(quark quark-version)
set_source_files_properties(${VERSION_FILE} GENERATED)

if(WIN32)
    set(WIN32_LIBRARIES
        Shcore
        ws2_32
        opengl32
        Pathcch
        Xinput
    )
elseif(APPLE)
    add_compile_options("-framework OpenGL")
else()
    set(POSIX_LIBRARIES
        GL
    )
endif()

target_link_libraries(quark
    # project libraries
    shared
    sound
    physics
    network
    # system libraries
    ${WIN32_LIBRARIES}
    ${POSIX_LIBRARIES}
)

target_include_directories(quark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        game
        render
        system
)

# Set up precompiled header
if(MSVC)
    set_source_files_properties(precompiled.cpp PROPERTIES COMPILE_FLAGS /Ycprecompiled.h OBJECT_OUTPUTS precompiled.pch)
    set_source_files_properties(${QUARK_SOURCES} PROPERTIES COMPILE_FLAGS /Yuprecompiled.h OBJECT_DEPENDS precompiled.pch)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${QUARK_SOURCES} precompiled.cpp precompiled.h)
source_group("\\" FILES precompiled.cpp precompiled.h)
