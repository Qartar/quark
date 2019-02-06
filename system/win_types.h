// win_types.h
//

#pragma once

// basetsd.h
#if defined(_WIN64)
typedef unsigned __int64 UINT_PTR;
typedef __int64 LONG_PTR;
#else
typedef unsigned int UINT_PTR;
typedef long LONG_PTR;
#endif // defined(_WIN64)

// winnt.h
typedef char CHAR;
typedef CHAR* LPSTR;

// windef.h
typedef struct tagRECT RECT;
typedef struct HWND__* HWND;
typedef struct HDC__* HDC;
typedef struct HGLRC__* HGLRC;

// minwindef.h
#define CALLBACK __stdcall
#define WINAPI __stdcall
#define APIENTRY WINAPI
typedef unsigned int UINT;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

// wtypes.h
typedef struct HINSTANCE__* HINSTANCE;
