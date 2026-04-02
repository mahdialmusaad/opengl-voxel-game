#pragma once
#ifndef SOURCE_DIRECTIVES_EXTERN_VXL_HDR
#define SOURCE_DIRECTIVES_EXTERN_VXL_HDR
/* C linkage directives. */

#if defined(__cplusplus)
/* C function declaration. */
# define VX_C_FUNC extern "C"
/* C declarations start. */
# define VX_C_START extern "C" {
/* C declarations end. */
# define VX_C_END }
/* Function has no arguments. */
# define VX_NO_ARG
#else /* C definitions below. */
/* C function declaration. */
# define VX_C_FUNC
/* C declarations start. */
# define VX_C_START
/* C declarations end. */
# define VX_C_END
/* Function has no arguments. */
# define VX_NO_ARG void
#endif

#endif
