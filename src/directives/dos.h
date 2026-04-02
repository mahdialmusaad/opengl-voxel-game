#pragma once
#ifndef SOURCE_DIRECTIVES_OS_VXL_HDR
#define SOURCE_DIRECTIVES_OS_VXL_HDR
/* Operating system directives. */

#if defined(__unix__) || defined(unix) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
# define VX_UNIX 1
# define _GNU_SOURCE 1
#else
# define VX_UNIX 0
#endif

#if defined(_WIN32) || defined(__WINDOWS__) || defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__) || defined(__CYGWIN__)
# define VX_WINDOWS 1
# define WIN32_LEAN_AND_MEAN
# define NOMINMAX
#else
# define VX_WINDOWS 0
#endif

#endif
