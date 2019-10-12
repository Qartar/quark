// r_window.h
//

#pragma once

#include "cm_config.h"
#include "win_types.h"

#if defined(USE_SDL)
typedef struct SDL_Window SDL_Window;
typedef void* SDL_GL_Context;
#endif // defined(USE_SDL)

////////////////////////////////////////////////////////////////////////////////
namespace render {

//------------------------------------------------------------------------------
class window
{
public:
    window(HINSTANCE hInstance, WNDPROC WndProc);
    ~window();

    result create();
    void destroy();
    void end_frame();

    LRESULT message(UINT uCmd, WPARAM wParam, LPARAM lParam);
    bool toggle_fullscreen();

#if defined(USE_SDL)
    SDL_Window* hwnd() const { return _hwnd; }
#else
    HWND hwnd() const { return _hwnd; }
    HDC hdc() const { return _hdc; }
#endif

    bool active() const { return _active; }
    vec2i position() const { return _position; }
    vec2i size() const { return _physical_size; }
    int width() const { return _physical_size.x; }
    int height() const { return _physical_size.y; }
    bool fullscreen() const { return _fullscreen; }

    render::system* renderer() { return &_renderer; }

private:
    result create(int xpos, int ypos, int width, int height, bool fullscreen);

    result init_opengl();
    void shutdown_opengl();

    result activate(bool active, bool minimized);
    void resize_for_dpi(RECT const* suggested, int dpi);

    bool _active;
#if defined(_WIN32)
    bool _minimized;
#endif // defined(_WIN32)
    bool _fullscreen;
    vec2i _position;
#if defined(_WIN32)
    vec2i _logical_size;
#endif // defined(_WIN32)
    vec2i _physical_size;

    config::integer _vid_width;
    config::integer _vid_height;
    config::boolean _vid_fullscreen;
    config::boolean _vid_vsync;

    render::system _renderer;

#if defined(USE_SDL)
    SDL_Window* _hwnd;
    SDL_GL_Context _hrc;

#else // !defined(USE_SDL)
    HWND _hwnd;
    HDC _hdc;

#if defined(_WIN32)
    HGLRC _hrc;

    HINSTANCE _hinst;
    WNDPROC _wndproc;

    int _current_dpi;
#endif // defined(_WIN32)

#endif // !defined(USE_SDL)
};

} // namespace render
