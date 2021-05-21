// gl_types.h
//

#pragma once

////////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
#define WINAPI __stdcall
#define APIENTRY WINAPI
#else
#define APIENTRY
#endif // defined(_WIN32)

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef double GLdouble;
typedef void GLvoid;

typedef char GLchar;

#if defined(_WIN64)
typedef signed long long int GLsizeiptr;
typedef signed long long int GLintptr;
#else
typedef signed long int GLsizeiptr;
typedef signed long int GLintptr;
#endif

//------------------------------------------------------------------------------
#if defined(USE_SDL)
using get_proc_address_t = void* (*)(char const* proc);
#elif defined(_WIN32)
#   if defined(_WIN64)
typedef long long INT_PTR;
#   else // !defined(_WIN64)
typedef int INT_PTR;
#   endif // defined(_WIN64)
typedef INT_PTR (WINAPI *PROC)();
using get_proc_address_t = PROC (WINAPI *)(char const* proc);
#endif // defined(_WIN32)
