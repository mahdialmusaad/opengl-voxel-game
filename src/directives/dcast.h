#pragma once
#ifndef SOURCE_DIRECTIVES_CAST_VXL_HDR
#define SOURCE_DIRECTIVES_CAST_VXL_HDR
/* Casting directives. */

#if defined(__cplusplus)
/* Reinterpret bytes as another type. */
# define VX_REINT_CAST(type, value) (reinterpret_cast<type>(value))
/* Cast type normally. */
# define VX_CAST(type, value) (static_cast<type>(value))
#else
/* Reinterpret bytes as another type. */
# define VX_REINT_CAST(type, value) ((type)(value))
/* Cast value to type normally. */
# define VX_CAST(type, value) ((type)(value))
#endif

#endif
