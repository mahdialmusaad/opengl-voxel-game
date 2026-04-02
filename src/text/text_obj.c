#include "text/text_obj.h"
#include "text/text_mgr.h"

#include "directives/dmath.h"
#include "directives/dcast.h"
#include "directives/dfree.h"
#include "directives/dword.h"

#include "shaders/textures.h"

#include "values/state.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Update the rendered items count of the given text object. */
static void vxtxt_obj_update_count(vxtxt_obj *text_obj)
{
	text_obj->displayed_elements = 0u;
	if (!text_obj->text) return;

	int displayed = 0;
	for (const char *txt_ptr = text_obj->text; *txt_ptr; ++txt_ptr) displayed += *txt_ptr >= '!';
	text_obj->displayed_elements = VX_CAST(unsigned short, displayed);

	if (text_obj->settings & vxen_txt_shadow) text_obj->displayed_elements *= 2u;
	if (text_obj->settings & vxen_txt_background) ++text_obj->displayed_elements;
}

#define VX_TEXT_FONT_MULT(tx) (tx->font_size / VX_TEXT_DEFAULT_FONT_SIZE)
#define VX_TEXT_CHAR_SPACING(tx) ((vxtxt_manager.text_width + tx->extra_char_spacing) * VX_TEXT_FONT_MULT(tx))
#define VX_TEXT_SPACE_SIZE(tx) (VX_TEXT_CHAR_SPACING(tx) * 4.0f)
#define VX_TEXT_LINE_SPACE(tx) ((vxtxt_manager.text_height + tx->extra_line_spacing) * VX_TEXT_FONT_MULT(tx))

void vxtxt_obj_init(
	vxtxt_obj *to_init,
	float pos_x, float pos_y,
	char *text, int do_copy,
	unsigned int settings,
	float font_size, float visibility
) {
	vxdy_array_add(&vxtxt_manager.text_obj_array, &to_init);
	memset(to_init, 0, sizeof *to_init);

	to_init->settings = VX_CAST(unsigned short, settings);
	to_init->font_size = font_size;
	to_init->pos_x = pos_x;
	to_init->pos_y = pos_y;

	vxtxt_obj_set_text(to_init, text, do_copy);
	vxtxt_obj_set_transparency(to_init, visibility);
}


void vxtxt_obj_set_text(vxtxt_obj *text_obj, char *text, int do_copy)
{
	if (text_obj->text) VX_FREE(text_obj->text);

	if (!text) {
		VX_FREE(text_obj->data);
		text_obj->displayed_elements = 0u;
	} else if (!do_copy) {
		text_obj->text = text;
	} else {
		const size_t given_txt_size = strlen(text) + 1u;
		text_obj->text = VX_CAST(char *, malloc(given_txt_size));
		if (!text_obj->text) return;
		memcpy(text_obj->text, text, given_txt_size);
	}

	vxtxt_obj_text_recalc(text_obj);
}
void vxtxt_obj_clear_text(vxtxt_obj *text_obj)
{
	vxtxt_obj_set_text(text_obj, VX_NULL, 0);
	vxtxt_obj_set_transparency(text_obj, 0);
}
void vxtxt_obj_text_recalc(vxtxt_obj *text_obj)
{
	vxtxt_obj_update_count(text_obj);
	vxtxt_obj_update_dims(text_obj);
	vxtxt_obj_dirty(text_obj);
}

float vxtxt_obj_get_transparency(vxtxt_obj *text_obj)
{
	if (text_obj->hide_time < 0.0f) return VX_CLAMP(text_obj->hide_time, -1.0f, 0.0f);
	return VX_CLAMP(text_obj->hide_time, 0.0f, 1.0f);
}
int vxtxt_obj_is_visible(vxtxt_obj *text_obj)
{
	return text_obj->displayed_transparency > 0.0f;
}
void vxtxt_obj_set_transparency(vxtxt_obj *text_obj, float transparency)
{
	text_obj->hide_time = -VX_CLAMP(transparency, 0.0f, 1.0f);
	text_obj->settings &= VX_CAST(unsigned short, ~vxen_txt_trnsp);
	vxtxt_obj_dirty(text_obj);
}
void vxtxt_obj_fade_timer(vxtxt_obj *text_obj, float after_seconds)
{
	text_obj->hide_time = after_seconds;
	text_obj->displayed_transparency = 1.0f;
	text_obj->settings |= vxen_txt_trnsp;
	vxtxt_obj_dirty(text_obj);
}

void vxtxt_obj_move_y_relativeto(vxtxt_obj *main, vxtxt_obj *target, float offset)
{
	main->pos_y = target->pos_y - target->internal_height - offset;
	vxtxt_obj_dirty(main);
}
void vxtxt_obj_move_x_relativeto(vxtxt_obj *main, vxtxt_obj *target, float offset)
{
	main->pos_x = target->pos_x + target->internal_width + offset;
	vxtxt_obj_dirty(main);
}

float vxtxt_obj_char_width(int char_ind, float font_multiplier)
{
	return VX_CAST(float, vxtxt_manager.char_sizes[char_ind]) * vxtxt_manager.text_width * font_multiplier;
}
float vxtxt_obj_char_height(vxtxt_obj *text_obj)
{
	return vxtxt_manager.text_height * VX_TEXT_FONT_MULT(text_obj);
}

void vxtxt_obj_update_dims(vxtxt_obj *text_obj)
{
	text_obj->internal_width = 0.0f;

	const float line_spc = VX_TEXT_LINE_SPACE(text_obj);
	if (text_obj->text) text_obj->internal_height = line_spc;
	else return;

	const float spc_size = VX_TEXT_SPACE_SIZE(text_obj);
	const float font_mult = VX_TEXT_FONT_MULT(text_obj);
	const float char_spc = VX_TEXT_CHAR_SPACING(text_obj);

	float accumulated_width = 0.0f;
	for (const char *text_ptr = text_obj->text; *text_ptr; ++text_ptr) {
		const char char_val = *text_ptr;
		if (char_val == '\n') {
			text_obj->internal_height += line_spc;
			text_obj->internal_width = fmaxf(accumulated_width, text_obj->internal_width);
			accumulated_width = 0.0f;
		} else if (char_val == ' ') accumulated_width += spc_size;
		else accumulated_width += vxtxt_obj_char_width(char_val - 33, font_mult) + char_spc;
	}

	if (text_obj->settings & vxen_txt_bg_full_width) text_obj->internal_width = 2.0f;
	else text_obj->internal_width = fmaxf(text_obj->internal_width, accumulated_width);
	 
	if (text_obj->settings & vxen_txt_background) {
		text_obj->internal_width += VX_TEXT_DEFAULT_BACKGROUND_OFFSET * 2.0f;
		text_obj->internal_height += VX_TEXT_DEFAULT_BACKGROUND_OFFSET * 2.0f;
	}
}
int vxtxt_obj_update(vxtxt_obj *text_obj)
{
	/* Do not update if the text should not be visible. */
	if (!text_obj->text || !text_obj->displayed_elements) return 0;
	if ((text_obj->settings & vxen_txt_debug) && !vxtg_toggles.debug_text) return 0;
	if ((text_obj->settings & vxen_txt_inventory_only) && !vxtg_toggles.inventory_open) return 0;

	/* If fading, wait until the timer ends to begin changing transparency. */
	const float time_passed = VX_CAST(float, vxstate_vals.frame_delta);
	int clean = 1;

	/* Decrease timer if waiting to fade. */
	if (text_obj->hide_time > 0.0f) {
		text_obj->hide_time -= time_passed;
		clean = 0;
		/* Set to -1 to begin decreasing from full transparency. */
		if (text_obj->hide_time <= 0.0f) text_obj->hide_time = -1.0f;
	}
	else if (!(text_obj->settings & vxen_txt_dirty)) return 1;
	/* Static/changing transparency is stored as the corresponding negative value. */
	if (text_obj->hide_time <= 0.0f) {
		if (text_obj->settings & vxen_txt_trnsp) {
			/* If fading out, stop doing it if the timer is going to become positive. */
			text_obj->settings ^= VX_CAST(unsigned short, VX_CAST(int, vxen_txt_trnsp) * ((text_obj->hide_time += time_passed) >= 0.0f));
			if (!(text_obj->settings & vxen_txt_trnsp)) text_obj->hide_time = 0.0f;
			clean = 0;
		}
		text_obj->displayed_transparency = -text_obj->hide_time;
	}

	const uint32_t col_alpha = VX_CAST(uint32_t, 255.0f * text_obj->displayed_transparency) << 24u;
	const uint32_t col_val = 0xFFFFFFu + col_alpha;

	/* Allocate space for char data, cancelling the update if it failed. */
	if (text_obj->data) VX_FREE(text_obj->data);
	text_obj->data = VX_CAST(vxtxt_obj_character *, malloc(sizeof *text_obj->data * text_obj->displayed_elements));
	if (!text_obj->data) return 0;

	const float font_height = vxtxt_obj_char_height(text_obj);
	float cur_pos_x = text_obj->pos_x, cur_pos_y = text_obj->pos_y;

	/* Current character quad index. Background uses index 0 (to render below text), so skip if adding one. */
	vxtxt_obj_character *text_buffer = text_obj->data + (text_obj->settings & vxen_txt_background);

	/* Precalculate sizes. */
	const float line_spc = VX_TEXT_LINE_SPACE(text_obj);
	const float spc_size = VX_TEXT_SPACE_SIZE(text_obj);
	const float font_mult = VX_TEXT_FONT_MULT(text_obj);
	const float char_spc = VX_TEXT_CHAR_SPACING(text_obj);

	for (const char *text_ptr = text_obj->text; *text_ptr;) {
		const char char_val = *text_ptr++;
		if (char_val == '\n') {
			cur_pos_x = text_obj->pos_x;
			cur_pos_y -= line_spc;
			continue;
		} else if (char_val == ' ') {
			cur_pos_x += spc_size;
			continue;
		}

		const int displayable_char_index = char_val - 33; /* Texture starts after the space character (exclamation mark, value 33). */
		const uint32_t displayable_char_bit_index = VX_CAST(uint32_t, displayable_char_index) * 2u; /* Texture list contains start and end, so actual index is double. */
		const float char_width = vxtxt_obj_char_width(displayable_char_index, font_mult);

		/* Create a shadow by using a black copy of character rendered underneath. */
		if (text_obj->settings & vxen_txt_shadow) {
			const vxtxt_obj_character shadow = {
				cur_pos_x + (vxtxt_manager.text_width * 0.5f), cur_pos_y - (font_height * vxtex_textures.text.pixel_height),
				char_width, font_height, displayable_char_bit_index, col_alpha
			};
			*text_buffer++ = shadow;			
		}

		const vxtxt_obj_character normal_char = {
			cur_pos_x, cur_pos_y, char_width, font_height,
			displayable_char_bit_index, col_val
		};
		*text_buffer++ = normal_char;
		cur_pos_x += char_width + char_spc;
	}

	/* Whilst fading, do not clear the 'dirty' bit until completed. */
	if (clean) text_obj->settings &= VX_CAST(unsigned short, ~vxen_txt_dirty);

	if (text_obj->settings & vxen_txt_background) {
		const vxtxt_obj_character bg_char = {
			(text_obj->settings & vxen_txt_bg_full_width) ? -1.0f : text_obj->pos_x - VX_TEXT_DEFAULT_BACKGROUND_OFFSET,
			text_obj->pos_y + VX_TEXT_DEFAULT_BACKGROUND_OFFSET,
			text_obj->internal_width, text_obj->internal_height,
			(sizeof vxtxt_manager.char_sizes - 1u) * 2u,
			127u + (127u << 8u) + (127u << 16u) + (VX_CAST(uint32_t, 128.0f * text_obj->displayed_transparency) << 24u)
		};
		*text_obj->data = bg_char;
	}

	return 1;
}

void vxtxt_obj_dirty(vxtxt_obj *to_mark)
{
	to_mark->settings |= vxen_txt_dirty;
	vxtxt_manager.any_update = 1;
}
