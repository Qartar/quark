// r_window.cpp
//

#include "precompiled.h"
#pragma hdrstop

#include "resource.h"

#include <ShellScalingApi.h>

namespace {

// additional opengl bindings
typedef BOOL (APIENTRY* PFNWGLSWAPINTERVALEXT)(GLint interval);
typedef int (APIENTRY* PFNWGLGETSWAPINTERVALEXT)();

static PFNWGLSWAPINTERVALEXT wglSwapIntervalEXT = NULL;
static PFNWGLGETSWAPINTERVALEXT wglGetSwapIntervalEXT = NULL;

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
namespace render {

constexpr int windowed_style = WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX;
constexpr int fullscreen_style = WS_OVERLAPPED;

//------------------------------------------------------------------------------
window::window(HINSTANCE hInstance, WNDPROC WndProc)
    : _current_dpi(USER_DEFAULT_SCREEN_DPI)
    , _vid_width("vid_width", 1280, config::archive | config::reset, "window width in logical pixels (dpi scaled)")
    , _vid_height("vid_height", 720, config::archive | config::reset, "window height in logical pixels (dpi scaled)")
    , _vid_fullscreen("vid_fullscreen", 0, config::archive | config::reset, "fullscreen window (uses desktop dimensions)")
    , _vid_vsync("vid_vsync", true, config::archive, "synchronize buffer swap to vertical blank (vsync)")
    , _renderer(this)
    , _hinst(hInstance)
    , _wndproc(WndProc)
    , _hwnd(nullptr)
    , _hdc(nullptr)
    , _hrc(nullptr)
{}

//------------------------------------------------------------------------------
result window::create()
{
    if (failed(create(CW_USEDEFAULT, 0, _vid_width, _vid_height, _vid_fullscreen))) {
        return result::failure;
    }

    return _renderer.init();
}

//------------------------------------------------------------------------------
void window::end_frame ()
{
    if (wglSwapIntervalEXT && _vid_vsync.modified()) {
        wglSwapIntervalEXT(_vid_vsync ? 1 : 0);
        _vid_vsync.reset();
    }

    SwapBuffers(_hdc);
}

//------------------------------------------------------------------------------
window::~window()
{
    if (_hwnd) {
        _renderer.shutdown();
        destroy();
    }
}

//------------------------------------------------------------------------------
result window::create(int xpos, int ypos, int width, int height, bool fullscreen)
{
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    // Setup struct for RegisterClass
    WNDCLASSW   wc;

    wc.style            = 0;
    wc.lpfnWndProc      = _wndproc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = _hinst;
    wc.hIcon            = LoadIconA(_hinst, MAKEINTRESOURCEA(IDI_ICON1));
    wc.hCursor          = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground    = 0;
    wc.lpszMenuName     = 0;
    wc.lpszClassName    = APP_CLASSNAME;

    if (!RegisterClassW(&wc)) {
        log::error("window::m_CreateWindow | RegisterClass failed\n");
        return result::failure;
    }

    // Create the Window

    int style = windowed_style;

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, style, FALSE);

    _hwnd = CreateWindowExW(
        0,
        APP_CLASSNAME,
        L"Quark",
        style,
        xpos,
        ypos,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL,
        _hinst,
        NULL);

    // Adjust for DPI
    {
        int dpi = GetDpiForWindow(_hwnd);

        rect.top = 0;
        rect.left = 0;
        rect.right = MulDiv(width, dpi, USER_DEFAULT_SCREEN_DPI);
        rect.bottom = MulDiv(height, dpi, USER_DEFAULT_SCREEN_DPI);

        AdjustWindowRectExForDpi(&rect, style, FALSE, 0, dpi);

        _physical_size.x = rect.right;
        _physical_size.y = rect.bottom;

        SetWindowPos(
            _hwnd,
            NULL,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        _current_dpi = dpi;
    }

    _logical_size.x = width;
    _logical_size.y = height;

    if (fullscreen) {
        _active = true;
        _minimized = false;
        _fullscreen = true;
    } else {
        _active = true;
        _minimized = false;
        _fullscreen = false;
    }

    if (_hwnd == NULL) {
        log::error("window::m_CreateWindow | CreateWindow failed\n");
        return result::failure;
    }

    if (fullscreen) {
        SetWindowLongPtrA(_hwnd, GWL_STYLE, fullscreen_style);
        ShowWindow(_hwnd, SW_SHOWMAXIMIZED);
    } else {
        ShowWindow(_hwnd, SW_SHOWNORMAL);
    }
    UpdateWindow(_hwnd);

    // initialize OpenGL
    if (failed(init_opengl())) {
        return result::failure;
    }

    // show the window
    SetForegroundWindow(_hwnd);
    SetFocus(_hwnd);

    return result::success;
}

//------------------------------------------------------------------------------
result window::init_opengl()
{
    int     pixelformat;
    PIXELFORMATDESCRIPTOR pfd = 
    {
        sizeof(PIXELFORMATDESCRIPTOR),  // size of this pfd
        1,                              // version number
        PFD_DRAW_TO_WINDOW |            // support window
        PFD_SUPPORT_OPENGL |            // support OpenGL
        PFD_DOUBLEBUFFER,               // double buffered
        PFD_TYPE_RGBA,                  // RGBA type
        24,                             // 24-bit color depth
        0, 0, 0, 0, 0, 0,               // color bits ignored
        0,                              // no alpha buffer
        0,                              // shift bit ignored
        0,                              // no accumulation buffer
        0, 0, 0, 0,                     // accum bits ignored
        24,                             // 24-bit z-buffer  
        8,                              // 8-bit stencil buffer
        0,                              // no auxiliary buffer
        PFD_MAIN_PLANE,                 // main layer
        0,                              // reserved
        0, 0, 0                         // layer masks ignored
    };

    // get DC
    if ((_hdc = GetDC(_hwnd)) == NULL) {
        log::error("window::m_InitGL | GetDC failed");
        return result::failure;
    }

    // select pixel format
    if ((pixelformat = ChoosePixelFormat(_hdc, &pfd)) == 0) {
        log::error("window::m_InitGL | ChoosePixelFormat failed");
        return result::failure;
    }
    if ((SetPixelFormat(_hdc, pixelformat, &pfd)) == FALSE) {
        log::error("window::m_InitGL | SetPixelFormat failed");
        return result::failure;
    }

    // set up context
    if ((_hrc = wglCreateContext(_hdc)) == 0) {
        log::error("window::m_InitGL: wglCreateContext failed");
        shutdown_opengl();
        return result::failure;
    }
    if (!wglMakeCurrent(_hdc, _hrc)) {
        log::error("window::m_InitGL: wglMakeCurrent failed");
        shutdown_opengl();
        return result::failure;
    }

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXT )wglGetProcAddress("wglSwapIntervalEXT");
    wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXT )wglGetProcAddress("wglGetSwapIntervalEXT");

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(_vid_vsync ? 1 : 0);
        _vid_vsync.reset();
    }

    return result::success;
}

//------------------------------------------------------------------------------
void window::destroy()
{
    if (_hwnd) {
        shutdown_opengl( );

        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
}

//------------------------------------------------------------------------------
void window::shutdown_opengl()
{
    wglMakeCurrent(NULL, NULL);

    if (_hrc) {
        wglDeleteContext(_hrc);
        _hrc = NULL;
    }

    if (_hdc) {
        ReleaseDC(_hwnd, _hdc);
        _hdc = NULL;
    }
}

//------------------------------------------------------------------------------
LRESULT window::message (UINT nCmd, WPARAM wParam, LPARAM lParam)
{
    switch (nCmd) {
        case WM_ACTIVATE:
            activate( (LOWORD(wParam) != WA_INACTIVE), ( HIWORD(wParam) > 0 ) );
            break;

        case WM_SIZE:
            _physical_size.x = LOWORD(lParam);
            _physical_size.y = HIWORD(lParam);
            // don't update renderer unless it's been initialized
            _renderer.resize(_physical_size); // uses params in WndParams
            break;

        case WM_MOVE:
            _position.x = (short)LOWORD(lParam); // horizontal position
            _position.y = (short)HIWORD(lParam); // vertical position
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_DPICHANGED: {
            resize_for_dpi(reinterpret_cast<RECT*>(lParam), HIWORD(wParam));
            break;
        }

        default:
            break;
    }

    return DefWindowProcW( _hwnd, nCmd, wParam, lParam );
}

//------------------------------------------------------------------------------
void window::resize_for_dpi(RECT const* suggested, int dpi)
{
    // By default Windows adjusts the window to preserve scale for the entire
    // window including the non-client area. This causes the dimensions of the
    // client area to change which we want to avoid.

    RECT new_border = {0, 0, 0, 0};
    RECT old_border = {0, 0, 0, 0};
    AdjustWindowRectExForDpi(&new_border, windowed_style, FALSE, 0, dpi);
    AdjustWindowRectExForDpi(&old_border, windowed_style, FALSE, 0, _current_dpi);

    _physical_size.x = MulDiv(_logical_size.x, dpi, USER_DEFAULT_SCREEN_DPI);
    _physical_size.y = MulDiv(_logical_size.y, dpi, USER_DEFAULT_SCREEN_DPI);

    SetWindowPos(
        _hwnd,
        NULL,
        suggested->left - old_border.left + new_border.left,
        suggested->top,
        _physical_size.x + new_border.right - new_border.left,
        _physical_size.y + new_border.bottom - new_border.top,
        SWP_NOZORDER | SWP_NOACTIVATE);

    _current_dpi = dpi;

    end_frame();
}

//------------------------------------------------------------------------------
bool window::toggle_fullscreen()
{
    if (_fullscreen) {
        ShowWindow(_hwnd, SW_RESTORE);
        SetWindowLongA(_hwnd, GWL_STYLE, windowed_style | WS_VISIBLE);
        _fullscreen = false;

        // reset the window dimensions. the resize message from above still
        // uses the fullscreen style and doesn't have the right dimensions
        {
            RECT rect, border = {}; GetWindowRect(_hwnd, &rect);
            AdjustWindowRectExForDpi(&border, windowed_style, FALSE, 0, GetDpiForWindow(_hwnd));
            _physical_size.x = rect.right - rect.left - (border.right - border.left);
            _physical_size.y = rect.bottom - rect.top - (border.bottom - border.top);
            _renderer.resize(_physical_size);
        }
    } else {
        SetWindowLongA(_hwnd, GWL_STYLE, fullscreen_style | WS_VISIBLE);
        ShowWindow(_hwnd, SW_MAXIMIZE);
        _fullscreen = true;
    }
    return true;
}

//------------------------------------------------------------------------------
result window::activate(bool active, bool minimized)
{
    if (active && !minimized) {
        if (!_active || _minimized) {
            SetForegroundWindow(_hwnd);
            if (_minimized) {
                ShowWindow(_hwnd, SW_RESTORE);
            }
            _active = true;
            _minimized = false;
        }
    } else {
        if (minimized) {
            _minimized = true;
            ShowWindow(_hwnd, SW_MINIMIZE);
        }

        _active = false;
    }

    return result::success;
}

} // namespace render
