#pragma once
#ifndef SOURCE_DIRECTIVES_OPTIMIZE_VXL_HDR
#define SOURCE_DIRECTIVES_OPTIMIZE_VXL_HDR
/* Optimization directives. */

/* Pragma directive. For internal use. */
#define VX_PRAGMA(str) _Pragma(#str)

#if !defined(_MSC_VER) && (defined(__clang__) || defined(__GNUC__))
/* Suggest to unroll the below loop a given number of times. */
# define VX_UNROLL(count) VX_PRAGMA(GCC unroll count)
#else
/* Suggest to unroll the below loop a given number of times. Not available. */
# define VX_UNROLL(count)
#endif

#if !defined(_MSC_VER) && (defined(__clang__) || defined(__GNUC__))
/* Force a function to be inline. */
# define VX_FORCEINLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
/* Force a function to be inline. */
#  define VX_FORCEINLINE __forceinline
#else
/* Force a function to be inline. Not fully available. */
# define VX_FORCEINLINE inline
#endif

#if !defined (_MSC_VER)
/* The given conditional is likely to be true. */
#define VX_LIKELY(cond) __builtin_expect(!!(cond), 1)
/* The given conditional is likely to be false. */
#define VX_UNLIKELY(cond) __builtin_expect(!!(cond), 0)
#else
/* The given conditional is likely to be true. */
#define VX_LIKELY(cond) (!!(cond))
/* The given conditional is likely to be false. */
#define VX_UNLIKELY(cond) (!!(cond))
#endif


#endif
