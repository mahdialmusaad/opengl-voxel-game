#pragma once
#ifndef SOURCE_DIRECTIVES_KEYWORDS_VXL_HDR
#define SOURCE_DIRECTIVES_KEYWORDS_VXL_HDR
/* Keyword macros. */

#if defined(__cplusplus)
/* Pointer does not alias. */
# define VX_RESTRICT __restrict
#else
/* Pointer does not alias. */
# define VX_RESTRICT restrict
#endif

#if (defined(__cplusplus) && __cplusplus > 199711L) || __STDC_VERSION__ >= 202311L
/* Null pointer. */
# define VX_NULL (nullptr)
#else
# include <stddef.h>
/* Null pointer. */
# define VX_NULL (NULL)
#endif

#endif
