/*
===============================================================================

Name    :   win_main.h

Purpose :   Windows API Interface

Date    :   10/15/2004

===============================================================================
*/

#pragma once

/*
===========================================================

Name    :   cWinApp

Purpose :   this is THE windows class winmain calls cWinApp::Main
            and everything is handled from within the class, wndproc
            is a static function for compatability issues.

            see win_main.cpp for explanation on member functions

===========================================================
*/

class manager;

class cWinApp
{
public:
    cWinApp (HINSTANCE hInstance);
    ~cWinApp () {}

    int     Main (LPSTR szCmdLine, int nCmdShow);

    int     Init (HINSTANCE hInstance, LPSTR szCmdLine);
    int     Shutdown ();

    void    Error (char *szTitle, char *szMessage);
    void    Quit (int nExitCode);

    char    *ClipboardData ();
    char    *InitString() { return m_szInitString; }

    // get_time : returns time in milliseconds
    float       get_time () { QueryPerformanceCounter( &m_timerCounter ) ; return ( (float)(m_timerCounter.QuadPart - m_timerBase.QuadPart) / (float)(m_timerFrequency.QuadPart) * 1000.0f ) ; }

    HINSTANCE   get_hInstance () { return m_hInstance; }
    render::window* window() { return &_window; }

private:
    HINSTANCE   m_hInstance;
    int         m_nExitCode;

    render::window _window;
    game::session _game;

    char        *m_szInitString;

    LARGE_INTEGER   m_timerFrequency;
    LARGE_INTEGER   m_timerCounter;
    LARGE_INTEGER   m_timerBase;

    static LRESULT m_WndProc (HWND hWnd, UINT nCmd, WPARAM wParam, LPARAM lParam);  // ATL SUCKS

    void        m_KeyEvent (int Param, bool Down);
    void        m_MouseEvent (int mstate);

    network::manager _network;
};

extern cWinApp *g_Application;
