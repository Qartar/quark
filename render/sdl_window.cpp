// sdl_window.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "sdl_window.h"
#include <SDL2/SDL_video.h>

namespace render {

//------------------------------------------------------------------------------
window::window(HINSTANCE hInstance, WNDPROC WndProc)
    : _vid_width("vid_width", 1280, config::archive | config::reset, "window width in logical pixels (dpi scaled)")
    , _vid_height("vid_height", 760, config::archive | config::reset, "window height in logical pixels (dpi scaled)")
    , _vid_fullscreen("vid_fullscreen", 0, config::archive | config::reset, "fullscreen window (uses desktop dimensions)")
    , _vid_vsync("vid_vsync", true, config::archive, "synchronize buffer swap to vertical blank (vsync)")
    , _renderer(this)
{}

//------------------------------------------------------------------------------
window::~window()
{
    if (_hwnd) {
        _renderer.shutdown();
        destroy();
    }
}

//------------------------------------------------------------------------------
result window::create()
{
    if (failed(create(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _vid_width, _vid_height, _vid_fullscreen))) {
        return result::failure;
    }

    return _renderer.init();
}

//------------------------------------------------------------------------------
result window::create(int xpos, int ypos, int width, int height, bool fullscreen)
{
    int style = SDL_WINDOW_OPENGL
        | SDL_WINDOW_HIDDEN
        | SDL_WINDOW_ALLOW_HIGHDPI
        | (fullscreen ? SDL_WINDOW_FULLSCREEN : 0);

    _hwnd = SDL_CreateWindow("Quark", xpos, ypos, width, height, style);
    if (!_hwnd) {
        log::error("SDL_CreateWindow() failed: %s\n", SDL_GetError());
        return result::failure;
    }

    SDL_GetWindowPosition(_hwnd, &_position.x, &_position.y);
    SDL_GetWindowSize(_hwnd, &_physical_size.x, &_physical_size.y);

    // initialize OpenGL
    if (failed(init_opengl())) {
        return result::failure;
    }

    SDL_ShowWindow(_hwnd);
    return result::success;
}

//------------------------------------------------------------------------------
void window::destroy()
{
    if (_hwnd) {
        shutdown_opengl();

        SDL_DestroyWindow(_hwnd);
        _hwnd = nullptr;
    }
}

//------------------------------------------------------------------------------
result window::init_opengl()
{
    // set up context
    if ((_hrc = SDL_GL_CreateContext(_hwnd)) == 0) {
        log::error("SDL_GL_CreateContext() failed: %s\n", SDL_GetError());
        shutdown_opengl();
        return result::failure;
    }

    if (SDL_GL_MakeCurrent(_hwnd, _hrc)) {
        log::error("SDL_GL_MakeCurrent() failed: %s\n", SDL_GetError());
        shutdown_opengl();
        return result::failure;
    }

    SDL_GL_SetSwapInterval(_vid_vsync ? 1 : 0);
    _vid_vsync.reset();

    return result::success;
}

//------------------------------------------------------------------------------
void window::shutdown_opengl()
{
    if (_hrc) {
        SDL_GL_MakeCurrent(_hwnd, nullptr);
        SDL_GL_DeleteContext(_hrc);
    }
}

//------------------------------------------------------------------------------
void window::end_frame()
{
    if (_vid_vsync.modified()) {
        SDL_GL_SetSwapInterval(_vid_vsync ? 1 : 0);
        _vid_vsync.reset();
    }

    SDL_GL_SwapWindow(_hwnd);
}

} // namespace render
