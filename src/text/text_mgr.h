#pragma once
#ifndef SOURCE_RENDERING_TEXTMGR_VXL_HDR
#define SOURCE_RENDERING_TEXTMGR_VXL_HDR
/* Text manager declarations. */

#include "directives/dextern.h"

#include "utils/dyarray.h"

/* Text manager object. */
extern struct vxtxt_mgr_obj
{
	/* Buffer of inventory text. */
	void *inv_text_buffer;
	/* Normalized width and height of the text.
	   Changing this requires updating all text objects. */
	float text_width, text_height;
	/* Limits on chat size. */
	int chat_line_char_limit, chat_lines_limit;
	/* Array of all created text objects. */
	vxdy_array text_obj_array;
	/* Sizes of characters in text texture. */
	const unsigned char char_sizes[95];
	unsigned char any_update : 1, inited : 7;
} vxtxt_manager;

/* The relative screen width of the 'default font size' (above). */
#define VX_TEXT_DEFAULT_FONT_SIZE_WIDTH (0.005f)
/* The relative screen height of the 'default font size'. */
#define VX_TEXT_DEFAULT_FONT_SIZE_HEIGHT (0.05f)

VX_C_START

/* Initialize render buffers of the global text renderer and the text shader uniform. */
void vxtxt_mgr_init(VX_NO_ARG);

/* Marks all text objects for vertex recalculation. */
void vxtxt_mgr_all_dirty(VX_NO_ARG);
/* Destroy all added text objects and the text manager's array. */
void vxtxt_mgr_destroy(VX_NO_ARG);

VX_C_END

#endif
