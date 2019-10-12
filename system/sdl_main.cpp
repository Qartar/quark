// sdl_main.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "sdl_main.h"
#include "cm_keys.h"
#include <SDL2/SDL.h>

#if !defined(_WIN32)
#   include <pwd.h>
#   include <unistd.h>
#endif // !defined(_WIN32)

//------------------------------------------------------------------------------
time_value time_value::current()
{
    static int64_t offset = SDL_GetPerformanceCounter();
    static double denominator = 1e-6 * static_cast<double>(SDL_GetPerformanceFrequency());
    double numerator = static_cast<double>(SDL_GetPerformanceCounter() - offset);
    return time_value::from_microseconds(static_cast<int64_t>(numerator / denominator));
}

////////////////////////////////////////////////////////////////////////////////
application* application::_singleton = nullptr;

//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    return application(nullptr).main(*argv, 0);
}

//------------------------------------------------------------------------------
application::application(HINSTANCE hInstance)
    : _hinstance(hInstance)
    , _exit_code(0)
    , _mouse_state(0)
    , _window(hInstance, nullptr)
{
    _singleton = this;
}

//------------------------------------------------------------------------------
int application::main(LPSTR szCmdLine, int /*nCmdShow*/)
{
    time_value previous_time, current_time;

    if (failed(init(_hinstance, szCmdLine))) {
        return shutdown();
    }

    previous_time = time_value::current();

    while (!SDL_QuitRequested()) {

        // inactive/idle loop
        if (!_window.active()) {
            SDL_Delay(1);
        }

        // message loop (pump)
#if 0
        SDL_PumpEvents();
#else
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            process_event(ev);
        }
#endif

        // gamepad events are polled from the device
        generate_gamepad_events();

        current_time = time_value::current();
        _game.run_frame(current_time - previous_time);
        previous_time = current_time;
    }

    return shutdown();
}

//------------------------------------------------------------------------------
result application::init(HINSTANCE hInstance, LPSTR szCmdLine)
{
    if (SDL_Init(SDL_INIT_VIDEO)) {
        log::error("SDL_Init() failed: %s\n", SDL_GetError());
        return result::failure;
    }

    // set instance
    _hinstance = hInstance;

    // set init string
    _init_string = string::view(szCmdLine);

    srand(static_cast<unsigned int>(SDL_GetPerformanceCounter()));

    _config.init();

    // initialize networking
#if defined(_WIN32)
    {
        WSADATA wsadata = {};
        if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        }
    }
#endif // defined(_WIN32)

    // create sound class
    if (failed(sound::system::create())) {
        // non-fatal, continue initialization
    }

    // init opengl
    if (failed(_window.create())) {
        return result::failure;
    }

    // init game
    if (failed(_game.init(_init_string))) {
        return result::failure;
    }

    return result::success;
}

//------------------------------------------------------------------------------
int application::shutdown()
{
    // shutdown opengl
    _window.destroy();

    // shutdown game
    _game.shutdown( );

    // shutdown sound
    sound::system::destroy();

    // shutdown networking
#if defined(_WIN32)
    WSACleanup();
#endif // defined(_WIN32)

    _config.shutdown();

#ifdef DEBUG_MEM    
    _CrtDumpMemoryLeaks();
#endif // DEBUG_MEM

    return _exit_code;
}

//------------------------------------------------------------------------------
void application::quit(int exit_code)
{
    // save the exit code for shutdown
    _exit_code = exit_code;

    // tell windows we dont want to play anymore
#if defined(_WIN32)
    PostQuitMessage(exit_code);
#else // !defined(_WIN32)
    SDL_Quit();
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
constexpr int keymap(SDL_Keycode c)
{
    switch(c)
    {
        case SDLK_F1: return K_F1;
        case SDLK_F2: return K_F2;
        case SDLK_F3: return K_F3;
        case SDLK_F4: return K_F4;
        case SDLK_F5: return K_F5;
        case SDLK_F6: return K_F6;
        case SDLK_F7: return K_F7;
        case SDLK_F8: return K_F8;
        case SDLK_F9: return K_F9;
        case SDLK_F10: return K_F10;
        case SDLK_F11: return K_F11;
        case SDLK_F12: return K_F12;

        case SDLK_RIGHT: return K_RIGHTARROW;
        case SDLK_LEFT: return K_LEFTARROW;
        case SDLK_DOWN: return K_DOWNARROW;
        case SDLK_UP: return K_UPARROW;

        case SDLK_KP_DIVIDE: return K_KP_SLASH;
        //case SDLK_KP_MULTIPLY: return K_KP_?
        case SDLK_KP_MINUS: return K_KP_MINUS;
        case SDLK_KP_PLUS: return K_KP_PLUS;
        case SDLK_KP_ENTER: return K_KP_ENTER;
        case SDLK_KP_5: return K_KP_5;

        case SDLK_LCTRL: return K_CTRL;
        case SDLK_LSHIFT: return K_SHIFT;
        case SDLK_LALT: return K_ALT;
        case SDLK_RCTRL: return K_CTRL;
        case SDLK_RSHIFT: return K_SHIFT;
        case SDLK_RALT: return K_ALT;

        default: return c;
    }
}

//------------------------------------------------------------------------------
void application::process_event(SDL_Event const& ev)
{
    switch (ev.type)
    {
        case SDL_WINDOWEVENT:
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            SDL_KeyboardEvent const& kev = reinterpret_cast<SDL_KeyboardEvent const&>(ev);
            _game.key_event(keymap(kev.keysym.sym), kev.state == SDL_PRESSED);
            break;
        }

        case SDL_MOUSEMOTION: {
            SDL_MouseMotionEvent const& mev = reinterpret_cast<SDL_MouseMotionEvent const&>(ev);
            _game.cursor_event(vec2(mev.x, mev.y));
            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            SDL_MouseButtonEvent const& mev = reinterpret_cast<SDL_MouseButtonEvent const&>(ev);
            _game.key_event(K_MOUSE1 + mev.button - 1, mev.state == SDL_PRESSED);
            break;
        }

        case SDL_MOUSEWHEEL: {
            SDL_MouseWheelEvent const& mev = reinterpret_cast<SDL_MouseWheelEvent const&>(ev);
            _game.key_event(mev.y > 0 ? K_MWHEELUP : K_MWHEELDOWN, true);
            _game.key_event(mev.y > 0 ? K_MWHEELUP : K_MWHEELDOWN, false);
            break;
        }
    }
}

//------------------------------------------------------------------------------
void application::char_event(WPARAM key, LPARAM /*state*/)
{
    _game.char_event(narrow_cast<int>(key));
}

//------------------------------------------------------------------------------
void application::generate_gamepad_events()
{
}

//------------------------------------------------------------------------------
string::buffer application::username() const
{
#if !defined(_WIN32)
    uid_t uid = geteuid();
    struct passwd* pw = getpwuid(uid);

    if (pw) {
      return string::buffer(pw->pw_name);
    } else {
        return string::buffer{};
    }
#else // !defined(_WIN32)
    return string::buffer{};
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
string::buffer application::clipboard() const
{
    char* c = SDL_GetClipboardText();
    string::buffer s(c);
    SDL_free(c);

    return s;
}
