#pragma once
#ifndef SOURCE_IO_FORMAT_VXL_HDR
#define SOURCE_IO_FORMAT_VXL_HDR
/* Text formatting functions. */

#include "directives/dextern.h"
#include "directives/dword.h"

/* No concat options. Use default behaviour of concat'ing to an allocated string. */
#define VX_FMT_CONCAT_DEFAULT (0u)
/* Store results in the first string for concatenation.
   The first string must be heap allocated or a null pointer. */
#define VX_FMT_CONCAT_RESULTS_FIRST (1u)

/* Use the exact number of decimals present in the number when converted to a string. */
#define VX_GROUP_ALL_DECIMALS -1
/* String depending on what the given value evaluates to. */
#define VX_FMT_BOOL(value) ((value) ? "true" : "false")

VX_C_START

/* Concatenates the two given strings.
   Returns an allocated string of the results or a null pointer on error.

   Optionally, you can choose to use the first string for the result.
   In this case, on error, a null pointer is returned and the first string is not changed.

   If the second string is a null pointer, a null pointer is returned with no other changes. */
char *vxfmt_concat_allocd(unsigned int bit_flags, char **VX_RESTRICT a, const char *VX_RESTRICT b);
/* Same as vxfmt_concat_allocd, but frees the second pointer afterwards. */
char *vxfmt_concat_allocd_free(unsigned int bit_flags, char **VX_RESTRICT a, char *VX_RESTRICT b);

/* Copies a string into an allocated pointer.
   Returns the allocated pointer, or a null pointer on error. */
char *vxfmt_copy_str(const char *to_copy);

/* Returns an allocated formatted string with the given format arguments.
   On error, a null pointer is returned. */
char *vxfmt_text(const char *fmt_string, ...);

/* Returns the number of occurrences of c in str. */
int vxfmt_count_char(const char *str, const char c);
/* Returns the nth index of the target character in the string.
   Returns a null pointer if not found. */
char *vxfmt_get_nth_found(char *str, const char find, int nth);

/* Returns whether the given string contains the given character. */
int vxfmt_contains_char(const char *str, const char c);

/* Returns whether the given string is a numerical value. */
int vxfmt_is_numeric(const char *num_str);

/* Same as strchr, but returns the string's terminator instead of a null pointer. */
char *vxfmt_strchrnul(char *str, char target);
/* Same as strchr, but returns the string's terminator instead of a null pointer.
   Const string return version. */
const char *vxfmt_const_strchrnul(const char *str, char target);

/* Returns a comma-separated version of the given number string with a maximum number of decimals. */
char *vxfmt_group_num(int decimals, char **result_str, const char *formatter_string, ...);
/* Format number with separator directly from number string. */
char *vxfmt_group_num_str(const char *as_str, char **result_str, int decimals);

VX_C_END

#endif
