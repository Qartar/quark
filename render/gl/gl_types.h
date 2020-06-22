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
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef char GLchar;
