#pragma once
#ifndef SOURCE_GRAPHICS_TYPES_VXL_HDR
#define SOURCE_GRAPHICS_TYPES_VXL_HDR
/* OpenGL types (modified from KHR). */

#include <stdint.h>

#if defined(__SIZEOF_LONG__) && defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ > __SIZEOF_LONG__)
typedef intptr_t GLintptr;
#elif defined(_WIN64)
typedef signed long long GLintptr;
#else
typedef signed long int GLintptr;
#endif

#if defined(_WIN64)
typedef signed long long GLsizeiptr;
#else
typedef signed long GLsizeiptr;
#endif

typedef char GLchar;
typedef signed char GLbyte;
typedef unsigned char GLubyte;
typedef unsigned char GLboolean;

typedef signed short GLshort;
typedef unsigned short GLushort;

typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned int GLenum;
typedef unsigned int GLbitfield;

typedef float GLfloat;
typedef double GLdouble;

typedef int64_t GLint64;
typedef uint64_t GLuint64;

#endif
