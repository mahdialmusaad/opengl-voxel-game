#pragma once
#ifndef SOURCE_DIRECTIVES_MATH_VXL_HDR
#define SOURCE_DIRECTIVES_MATH_VXL_HDR
/* Mathematical directives. */

/* Pi. */
#define VX_PI (3.141592653589793238462643383279502884197169399)
/* Pi multiplied by 2. */
#define VX_TWO_PI (6.283185307179586476925286766559005768394338798)

/* Square root of 3. */
#define VX_SQRT_3 (1.732050807568877293527446341505872366942805253)
/* Square root of 2. */
#define VX_SQRT_2 (1.414213562373095048801688724209698078569671875)

/* Multiply with a degree value to get the corresponding radian value. */
#define VX_RADIAN_MULT (VX_PI / 180.0)
/* Multiply with a radian value to get the corresponding degree value. */
#define VX_DEGREE_MULT (180.0 / VX_PI)

/* Clamp X between min and max. */
#define VX_CLAMP(x, min, max) (((x) > (max)) ? (max) : (((x) < (min)) ? (min) : (x)))
/* Ternary for checking inclusive range of x. */
#define VX_BETWEEN(x, min, max) ((x) >= (min) && (x) <= (max))
/* Ternary for checking exclusive range of x. */
#define VX_BETWEEN_EXCL(x, min, max) ((x) > (min) && (x) < (max))

/* Integer minimum. */
#define VX_INT_MIN(a, b) ((a) < (b) ? (a) : (b))
/* Integer maximum. */
#define VX_INT_MAX(a, b) ((a) > (b) ? (a) : (b))

/* Sign value of x, either 0 or 1. */
#define VX_ZERO_ONE_SIGN(x) ((x) < 0 ? 0 : 1)
/* Sign value of x, either -1 or 1. */
#define VX_NEG_ONE_SIGN(x) ((x) < 0 ? -1 : 1)

#endif
