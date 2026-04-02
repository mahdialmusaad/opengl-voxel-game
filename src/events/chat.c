#include "events/chat.h"

#include "directives/dcast.h"
#include "directives/dword.h"
#include "directives/dfree.h"

#include "text/text_mgr.h"

#include "io/format.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

vxtxt_obj vxchat_text;

void vxchat_add_text(const char *append)
{
	if (!append || !*append) return;
	const size_t current_len = vxchat_text.text ? strlen(vxchat_text.text) : 0u;
	const size_t append_len = strlen(append);
	const size_t add_nl = current_len != 0u;

	char *result_text = VX_CAST(char *, malloc(current_len + add_nl + append_len + 1u));
	if (!result_text) return;

	/* Copy original text to start of string. */
	memcpy(result_text, vxchat_text.text, current_len);
	if (add_nl) result_text[current_len] = '\n';
	result_text[current_len + append_len + add_nl] = '\0';

	/* Copy and format 'append' string at the end of the original text. */
	memcpy(result_text + current_len + add_nl, append, append_len);
	vxchat_fmt_chat(result_text + current_len + add_nl);

	/* Shift text backwards to replace beginning lines if the chat text is too large. */
	const int lines_to_replace = vxfmt_count_char(result_text, '\n') - vxtxt_manager.chat_lines_limit;
	char *to_replace_start = vxfmt_get_nth_found(result_text, '\n', lines_to_replace);
	memmove(result_text, to_replace_start, VX_CAST(size_t, to_replace_start - result_text));

	vxtxt_obj_set_text(&vxchat_text, result_text, 0);
}

void vxchat_add_text_free(char *append)
{
	vxchat_add_text(append);
	VX_FREE(append);
}

char *vxchat_fmt_chat(char *to_fmt)
{
	if (!to_fmt || !*to_fmt) return VX_NULL;

	char *previous_space = to_fmt;
	int line_characters = 0;

	do {
		char *found_space = vxfmt_strchrnul(previous_space + 1, ' ');
		line_characters = VX_CAST(int, found_space - to_fmt);

		/* Found space is further than the maximum distance, add line at previous space. */
		if (line_characters > vxtxt_manager.chat_line_char_limit) {
			/* Wrap line repeatedly instead if previous space is too far away. */
			if (VX_CAST(int, found_space - previous_space) > vxtxt_manager.chat_line_char_limit) {
				do *(previous_space += vxtxt_manager.chat_line_char_limit) = '\n';
				while (VX_CAST(int, found_space - previous_space) > vxtxt_manager.chat_line_char_limit);
			} else *previous_space = '\n';
			line_characters = 0;
			to_fmt = found_space;
		} else if (line_characters == vxtxt_manager.chat_line_char_limit) {
			/* Found at exactly the maximum distance, add line at current space. */
			*found_space = '\n';
			line_characters = 0;
			to_fmt = found_space;
		}

		if (!*found_space) break;
		previous_space = found_space;
	} while (1);

	return to_fmt;
}
