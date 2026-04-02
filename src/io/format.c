#include "io/format.h"

#include "directives/dcast.h"
#include "directives/dfree.h"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(va_copy)
# define va_copy(a, b) do memcpy(a, b, sizeof a); while (0)
#endif

char *vxfmt_concat_allocd(unsigned int bit_flags, char **VX_RESTRICT a, const char *VX_RESTRICT b)
{
	if (!a || !b) return VX_NULL;
	
	const size_t a_len = (*a ? strlen(*a) : 0u);
	const size_t b_len = strlen(b);
	const size_t res_len = a_len + b_len + 1u;
	char *res;
	
	if (bit_flags & VX_FMT_CONCAT_RESULTS_FIRST) {
		char *realloc_res = VX_CAST(char *, realloc(*a, res_len));
		if (realloc_res != *a) res = (*a = realloc_res);
		else if (!realloc_res) return VX_NULL;
		else res = *a;
	} else if (!(res = VX_CAST(char *, malloc(res_len)))) return VX_NULL;
	else if (a_len) memcpy(res, *a, a_len);
	memcpy(res + a_len, b, b_len);
	res[a_len + b_len] = '\0';
	return res;
}
char *vxfmt_concat_allocd_free(unsigned int bit_flags, char **VX_RESTRICT a, char *VX_RESTRICT b)
{
	char *res = vxfmt_concat_allocd(bit_flags, a, b);
	VX_FREE(b);
	return res;
}

char *vxfmt_copy_str(const char *to_copy)
{
	const size_t copy_length_terminated = strlen(to_copy) + 1u;
	return VX_CAST(char *, memcpy(malloc(copy_length_terminated), to_copy, copy_length_terminated));
}

int vxfmt_count_char(const char *str, const char c)
{
	if (!str) return 0;
	int count = 0;
	while (*str) count += *str++ == c;
	return count;
}

char *vxfmt_get_nth_found(char *str, const char find, int nth)
{
	if (nth < 1 || !str) return str;
	char *parser = str;
	for (; *parser && nth; ++parser) nth -= (*parser == find);
	return parser;
}

int vxfmt_contains_char(const char *str, const char c)
{
	if (!str) return 0;
	while (*str && *str != c) ++str;
	return *str == c;
}


int vxfmt_is_numeric(const char *num_str)
{
	if (!num_str || !*num_str) return 0;
	if (*num_str == '+' || *num_str == '-') ++num_str;

	const size_t len = strlen(num_str);

	const char *found_exponent = VX_NULL;
	int decms_found = 0;

	for (size_t cur_index = 0u; cur_index < len; ++cur_index) {
		if (*num_str == '.') {
			if (++decms_found > 1) return 0;
			continue;
		}
		else if (!found_exponent && (*num_str == 'e' || *num_str == 'E') && cur_index) { found_exponent = num_str; continue; }
		else if ((*num_str == '+' || *num_str == '-') && (num_str - found_exponent != 1)) return 0;
		
		if (*num_str < '0' || *num_str > '9') return 0;
	}

	return 1;
}


char *vxfmt_strchrnul(char *str, char target)
{
	if (!str) return VX_NULL;
	char *parser = str;
	for (; *parser && *parser != target; ++parser);
	return parser;
}
const char *vxfmt_const_strchrnul(const char *str, char target)
{
	if (!str) return VX_NULL;
	const char *parser = str;
	for (; *parser && *parser != target; ++parser);
	return parser;
}


/* Internal format function. */
static char *vxfmt_text_valist(const char *fmt_string, va_list *args)
{
	va_list length_args;
	va_copy(length_args, (*args));

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

	const int fmtd_sz = vsnprintf(VX_NULL, 0, fmt_string, length_args);
	va_end(length_args);

	if (fmtd_sz < 0) return VX_NULL;
	char *fmt_buf = VX_CAST(char *, malloc(VX_CAST(size_t, fmtd_sz) + 1u));
	if (!fmt_buf) return VX_NULL;

	const int vsn_res = vsnprintf(fmt_buf, VX_CAST(size_t, fmtd_sz) + 1u, fmt_string, *args);

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

	if (vsn_res < 0) {
		VX_FREE(fmt_buf);
		return VX_NULL;
	}

	return fmt_buf;
}

char *vxfmt_text(const char *fmt_string, ...)
{
	if (!fmt_string) return VX_NULL;

	va_list args;
	va_start(args, fmt_string);

	char *fmtd_str = vxfmt_text_valist(fmt_string, &args);

	va_end(args);
	return fmtd_str;
}

char *vxfmt_group_num(int decimals, char **result_str, const char *formatter_string, ...)
{
	va_list args;
	va_start(args, formatter_string);

	if (*result_str) VX_FREE(*result_str);

	char *as_str = vxfmt_text_valist(formatter_string, &args);
	vxfmt_group_num_str(as_str, result_str, decimals);

	VX_FREE(as_str);
	va_end(args);

	return *result_str;
}

char *vxfmt_group_num_str(const char *as_str, char **result_str, int decimals)
{
	/* Values that aren't represented with numbers (e.g. nan, inf) should just be copied. */
	if (*as_str != '-' && *as_str < '0' && *as_str > '9') {
		*result_str = vxfmt_copy_str(as_str);
		return *result_str;
	}

	/* Determine properties and sizes relating to the given number string. */
	const char *decimal_point_ptr = vxfmt_const_strchrnul(as_str, '.');
	const char *terminator_ptr = as_str + strlen(as_str);
	const size_t has_decimal_point = decimal_point_ptr != terminator_ptr;
	const size_t has_negative_sign = *as_str == '-';

	const size_t num_decimals = has_decimal_point ? VX_CAST(size_t, (terminator_ptr - decimal_point_ptr) - 1) : 0u;
	const size_t integral_part_length = VX_CAST(size_t, decimal_point_ptr - (as_str + has_negative_sign));
	
	size_t decimals_wanted = (decimals == VX_GROUP_ALL_DECIMALS) ? num_decimals : VX_CAST(size_t, decimals);
	const size_t comma_separators = ((integral_part_length - 1u) / 3u);
	const size_t fmt_bytes = has_negative_sign + integral_part_length + comma_separators + has_decimal_point + decimals_wanted;

	/* Allocate the exact number of bytes needed for the string. */
	char *grouped_str = VX_CAST(char *, malloc(fmt_bytes + 1u));
	if (!grouped_str) return VX_NULL;
	if (has_negative_sign) *grouped_str = *as_str++;
	grouped_str[fmt_bytes] = '\0';

	/* Copy integer part of number string, adding a comma every three passes (with checks to avoid adding an early comma). */
	size_t actual_index = has_negative_sign;
	for (size_t grouped_index = 3u - (integral_part_length % 3u); as_str != decimal_point_ptr; ++as_str) {
		if (comma_separators && (actual_index != has_negative_sign) && (++grouped_index % 3u) == 0u) grouped_str[actual_index++] = ',';
		grouped_str[actual_index++] = *as_str;
	}

	/* Copy decimal point and decimal values, inserting '0' if there aren't enough in the original string. */
	if (has_decimal_point && (decimals != 0)) {
		grouped_str[actual_index++] = *as_str++;
		size_t decimals_inserted = 0u;
		while (decimals_inserted < decimals_wanted) grouped_str[actual_index++] = (decimals_inserted++ < num_decimals) ? *as_str++ : '0';
	}

	*result_str = grouped_str;
	return grouped_str;
}
