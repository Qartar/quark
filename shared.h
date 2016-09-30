/*
===============================================================================

Name	:	shared.h

Purpose	:	Global Definitions ; included first by all header files

Date	:	10/16/2004

===============================================================================
*/

#define WIN32_LEAN_AND_MEAN		// exclude rarely used Windows crap

#define APP_CLASSNAME		"Tanks!"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#pragma warning (disable:4244)

#define ERROR_NONE		0
#define ERROR_FAIL		1
#define ERROR_UNKNOWN	-1

#ifndef NULL
#define NULL 0
#endif // NULL

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define DEG2RAD(a) (a*M_PI)/180.0F

#define frand() (((float)rand())/32767.0f)
#define crand() ((frand()-0.5f)*2)

typedef class cVec2
{
public:
	float	x, y;

	cVec2 () {}
	cVec2 (float X, float Y) : x(X), y(Y) {}

	float operator[] (int i) { return ( *((float*)(this)+i) ); }
	cVec2 rot (float deg) {
		return cVec2(
			( x*cos(DEG2RAD(deg)) - y*sin(DEG2RAD(deg)) ),
			( y*cos(DEG2RAD(deg)) + x*sin(DEG2RAD(deg)) ) ); }

	cVec2 operator+ (cVec2 &A) { return cVec2(x + A.x,y + A.y); }
	cVec2 operator- (cVec2 &A) { return cVec2(x - A.x,y - A.y); }
	cVec2 operator* (float fl) { return cVec2(x * fl, y * fl); }

} vec2;

typedef class cVec4
{
public:
	float	r, g, b, a;

	cVec4 () {}
	cVec4 (float R, float G, float B, float A) : r(R), g(G), b(B), a(A) {}

	float operator[] (int i) { return ( *((float*)(this)+i) ); }
} vec4;

#define min(a,b) ( a < b ? a : b )
#define max(a,b) ( a > b ? a : b )
#define clamp(a,b,c) ( a < b ? b : ( a > c ? c : a ) )

static char	*va(char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, format);
	vsprintf (string, format,argptr);
	va_end (argptr);

	return string;	
}