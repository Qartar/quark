// win_main.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "cm_keys.h"

#if defined(_WIN32)
#   include <WS2tcpip.h>
#   include <XInput.h>
#endif // defined(_WIN32)

////////////////////////////////////////////////////////////////////////////////
namespace {

int64_t get_ticks()
{
#if defined(_WIN32)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
#else // !defined(_WIN32)
    return 0;
#endif // !defined(_WIN32)
}

int64_t get_ticks_per_second()
{
#if defined(_WIN32)
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
#else // !defined(_WIN32)
    return 0;
#endif // !defined(_WIN32)
}

} // anonymous namespace

//------------------------------------------------------------------------------
time_value time_value::current()
{
    static int64_t offset = get_ticks();
    static double denominator = 1e-6 * static_cast<double>(get_ticks_per_second());
    double numerator = static_cast<double>(get_ticks() - offset);
    return time_value::from_microseconds(static_cast<int64_t>(numerator / denominator));
}

////////////////////////////////////////////////////////////////////////////////
application* application::_singleton = nullptr;

//------------------------------------------------------------------------------
#if defined(_WIN32)
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR szCmdLine, int nCmdShow)
{
    return application(hInstance).main(szCmdLine, nCmdShow);
}
#else
int main(int argc, char** argv)
{
    return application(0).main(nullptr, 0);
}
#endif // defined(_WIN32)

//------------------------------------------------------------------------------
application::application(HINSTANCE hInstance)
    : _hinstance(hInstance)
    , _exit_code(0)
    , _mouse_state(0)
    , _window(hInstance, wndproc)
{
    _singleton = this;
}

//------------------------------------------------------------------------------
int application::main(LPSTR szCmdLine, int /*nCmdShow*/)
{
    time_value previous_time, current_time;
#if defined(_WIN32)
    MSG msg;
#endif // defined(_WIN32)

    if (failed(init(_hinstance, szCmdLine))) {
        return shutdown();
    }

    previous_time = time_value::current();

    while (true) {

        // inactive/idle loop
        if (!_window.active()) {
#if defined(_WIN32)
            Sleep(1);
#endif // defined(_WIN32)
        }

        // message loop (pump)
#if defined(_WIN32)
        while (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (!GetMessageW(&msg, NULL, 0, 0 )) {
                return shutdown();
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
#endif // defined(_WIN32)

        // gamepad events are polled from the device
        generate_gamepad_events();

        current_time = time_value::current();
        _game.run_frame(current_time - previous_time);
        previous_time = current_time;
    }
}

//------------------------------------------------------------------------------
result application::init(HINSTANCE hInstance, LPSTR szCmdLine)
{
    // set instance
    _hinstance = hInstance;

    // set init string
    _init_string = string::view(szCmdLine);

    srand(static_cast<unsigned int>(get_ticks()));

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
    exit(exit_code);
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
LRESULT application::wndproc(HWND hWnd, UINT nCmd, WPARAM wParam, LPARAM lParam)
{
#if defined(_WIN32)
    switch (nCmd)
    {
    case WM_NCCREATE:
        EnableNonClientDpiScaling(hWnd);
        return DefWindowProcW( hWnd, nCmd, wParam, lParam );

    case WM_CREATE:
        pSound->on_create(hWnd);
        return DefWindowProcW( hWnd, nCmd, wParam, lParam );

    case WM_CLOSE:
        _singleton->quit(0);
        return 0;

    // Game Messages

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE: {
        POINT P{(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)};
        _singleton->mouse_event(wParam, vec2i(P.x, P.y));
        break;
    }

    case WM_MOUSEWHEEL: {
        POINT P{(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)};
        ScreenToClient(hWnd, &P);
        _singleton->mouse_event(wParam, vec2i(P.x, P.y));
        break;
    }

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN)) {
            return 0;
        }
        _singleton->key_event( lParam, true );
        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        _singleton->key_event( lParam, false );
        break;

    case WM_SYSCHAR:
    case WM_CHAR:
        if (wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN)) {
            _singleton->_window.toggle_fullscreen();
            return 0;
        }
        _singleton->char_event(wParam, lParam);
        break;

    // glWnd Messages

    case WM_ACTIVATE:
    case WM_SIZE:
    case WM_MOVE:
    case WM_DESTROY:
    case WM_DPICHANGED:
        return _singleton->_window.message( nCmd, wParam, lParam );

    default:
        break;
    }

    return DefWindowProcW(hWnd, nCmd, wParam, lParam);
#else // !defined(_WIN32)
    return 0;
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
void application::char_event(WPARAM key, LPARAM /*state*/)
{
    _game.char_event(narrow_cast<int>(key));
}

//------------------------------------------------------------------------------
constexpr unsigned char keymap[128] = {
    0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6',
    '7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0
    'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
    'o',    'p',    '[',    ']',    13 ,    K_CTRL, 'a',    's',      // 1
    'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
    '\'' ,  '`',    K_SHIFT,'\\',   'z',    'x',    'c',    'v',      // 2
    'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*',
    K_ALT,  ' ',    0  ,    K_F1,   K_F2,   K_F3,   K_F4,   K_F5,   // 3
    K_F6,   K_F7,   K_F8,   K_F9,   K_F10,  K_PAUSE,    0  ,K_HOME,
    K_UPARROW,  K_PGUP, K_KP_MINUS, K_LEFTARROW,K_KP_5, K_RIGHTARROW,   K_KP_PLUS,  K_END, //4
    K_DOWNARROW,K_PGDN, K_INS,      K_DEL,  0,      0,      0,  K_F11,
    K_F12,  0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6
    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7
};

//------------------------------------------------------------------------------
void application::key_event(LPARAM param, bool down)
{
    int result;
    LPARAM modified = ( param >> 16 ) & 255;
    bool is_extended = false;

    if ( modified > 127) {
        return;
    }

    if ( param & ( 1 << 24 ) ) {
        is_extended = true;
    }

    result = keymap[modified];

    if (!is_extended) {
        switch (result) {
            case K_HOME:
                result = K_KP_HOME;
                break;
            case K_UPARROW:
                result = K_KP_UPARROW;
                break;
            case K_PGUP:
                result = K_KP_PGUP;
                break;
            case K_LEFTARROW:
                result = K_KP_LEFTARROW;
                break;
            case K_RIGHTARROW:
                result = K_KP_RIGHTARROW;
                break;
            case K_END:
                result = K_KP_END;
                break;
            case K_DOWNARROW:
                result = K_KP_DOWNARROW;
                break;
            case K_PGDN:
                result = K_KP_PGDN;
                break;
            case K_INS:
                result = K_KP_INS;
                break;
            case K_DEL:
                result = K_KP_DEL;
                break;
            default:
                break;
        }
    } else {
        switch (result) {
            case 0x0D:
                result = K_KP_ENTER;
                break;
            case 0x2F:
                result = K_KP_SLASH;
                break;
            case 0xAF:
                result = K_KP_PLUS;
                break;
        }
    }

    _game.key_event(result, down);
}

//------------------------------------------------------------------------------
void application::mouse_event(WPARAM mouse_state, vec2i position)
{
    for (int ii = 0; ii < 3; ++ii) {
        if ((mouse_state & (1LL << ii)) && !(_mouse_state & (1LL << ii))) {
            _game.key_event (K_MOUSE1 + ii, true);
        }

        if (!(mouse_state & (1LL << ii)) && (_mouse_state & (1LL << ii))) {
            _game.key_event (K_MOUSE1 + ii, false);
        }
    }

#if defined(_WIN32)
    if ((int16_t)HIWORD(mouse_state) < 0) {
        _game.key_event(K_MWHEELDOWN, true);
        _game.key_event(K_MWHEELDOWN, false);
    } else if ((int16_t)HIWORD(mouse_state) > 0) {
        _game.key_event(K_MWHEELUP, true);
        _game.key_event(K_MWHEELUP, false);
    }
#endif // defined(_WIN32)

    _mouse_state = mouse_state;
    _game.cursor_event(vec2(position));
}

//------------------------------------------------------------------------------
void application::generate_gamepad_events()
{
#if defined(_WIN32)
    for (DWORD ii = 0; ii < XUSER_MAX_COUNT; ++ii) {
        XINPUT_STATE state{};

        DWORD dwResult = XInputGetState(ii, &state);
        if (dwResult != ERROR_SUCCESS) {
            continue;
        }

        constexpr DWORD thumb_maximum = 32767;
        constexpr DWORD thumb_deadzone =
            (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) / 2;

        game::gamepad pad{};

        pad.thumbstick[game::gamepad::left] = {
            (float)state.Gamepad.sThumbLX, (float)state.Gamepad.sThumbLY
        };

        pad.thumbstick[game::gamepad::right] = {
            (float)state.Gamepad.sThumbRX, (float)state.Gamepad.sThumbRY
        };

        pad.trigger[game::gamepad::left] = state.Gamepad.bLeftTrigger * (1.0f / 255.0f);
        pad.trigger[game::gamepad::right] = state.Gamepad.bRightTrigger * (1.0f / 255.0f);

        for (int side = 0; side < 2; ++side) {
            float thumb_magnitude = pad.thumbstick[side].length();
            if (thumb_magnitude < thumb_deadzone) {
                pad.thumbstick[side] = vec2_zero;
            } else {
                pad.thumbstick[side].normalize_self();
                if (thumb_magnitude < thumb_maximum) {
                    pad.thumbstick[side] *= (thumb_magnitude - thumb_deadzone) / (thumb_maximum - thumb_deadzone);
                }
            }
        }

        // emit gamepad event
        _game.gamepad_event(ii, pad);

        // process buttons
        XINPUT_KEYSTROKE keystroke{};

        while (XInputGetKeystroke(ii, 0, &keystroke) == ERROR_SUCCESS) {
            _game.key_event(keystroke.VirtualKey, !(keystroke.Flags & XINPUT_KEYSTROKE_KEYUP));
        }
    }
#endif // defined(_WIN32)
}

//------------------------------------------------------------------------------
string::buffer application::username() const
{
#if defined(_WIN32)
    string::buffer s;
    constexpr DWORD buffer_size = 1024;
    wchar_t buffer[buffer_size];
    DWORD length = buffer_size;

    if (GetUserNameW(buffer, &length)) {
        // Calculate the destination size in bytes
        int len = WideCharToMultiByte(
            CP_UTF8,
            0,
            buffer,
            length,
            nullptr,
            0,
            NULL,
            NULL);

        // Convert directly into string::buffer's data
        s.resize(len - 1);
        WideCharToMultiByte(
            CP_UTF8,
            0,
            buffer,
            length,
            s.data(),
            narrow_cast<int>(s.length()),
            NULL,
            NULL);
    }

    return s;
#else // !defined(_WIN32)
    return string::buffer{};
#endif // !defined(_WIN32)
}

//------------------------------------------------------------------------------
string::buffer application::clipboard() const
{
#if defined(_WIN32)
    string::buffer s;

    if (OpenClipboard(NULL) != 0) {
        HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);

        if (hClipboardData != NULL) {
            wchar_t* cliptext = (wchar_t *)GlobalLock(hClipboardData);

            if (cliptext) {
                // Calculate the destination size in bytes
                int len = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    cliptext,
                    -1,
                    nullptr,
                    0,
                    NULL,
                    NULL);

                // Convert directly into string::buffer's data
                s.resize(len - 1);
                WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    cliptext,
                    -1,
                    s.data(),
                    narrow_cast<int>(s.length()),
                    NULL,
                    NULL);

                GlobalUnlock(hClipboardData);
            }
        }
        CloseClipboard();
    }

    return s;
#else // !defined(_WIN32)
    return string::buffer{};
#endif // !defined(_WIN32)
}
