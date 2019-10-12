// win_main.h
//

#pragma once

#include "win_types.h"

typedef union SDL_Event SDL_Event;

//------------------------------------------------------------------------------
class application
{
public:
    application(HINSTANCE hInstance);

    int main(LPSTR szCmdLine, int nCmdShow);

    void quit(int exit_code);

    string::buffer username() const;
    string::buffer clipboard() const;
    string::view init_string() { return _init_string; }

    HINSTANCE hinstance() { return _hinstance; }
    config::system* config() { return &_config; }
    render::window* window() { return &_window; }

    static application* singleton() { return _singleton; }

protected:
    HINSTANCE _hinstance;
    int _exit_code;

    WPARAM _mouse_state;

    string::view _init_string;

    render::window _window;
    game::session _game;
    config::system _config;

    static application* _singleton;

protected:
    result init(HINSTANCE hInstance, LPSTR szCmdLine);
    int shutdown();

    void char_event(WPARAM key, LPARAM state);
    void key_event(LPARAM param, bool down);
    void mouse_event(WPARAM mouse_state, vec2i position);

    void generate_gamepad_events();

#if defined(USE_SDL)
    void process_event(SDL_Event const& ev);
#else
    static LRESULT CALLBACK wndproc(HWND hWnd, UINT nCmd, WPARAM wParam, LPARAM lParam);
#endif
};
