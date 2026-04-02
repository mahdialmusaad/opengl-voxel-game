#pragma once
#ifndef SOURCE_RENDERING_TEXTOBJ_VXL_HDR
#define SOURCE_RENDERING_TEXTOBJ_VXL_HDR
/* Text object declarations. */

#include "directives/dextern.h"

#include <stdint.h>

/* Text settings bit values. */
enum
{
	/* Add a background to this text. */
	vxen_txt_background = 1,
	/* The background should take up the entire screen width. */
	vxen_txt_bg_full_width = 2,
	/* The text should have a shadow. */
	vxen_txt_shadow = 4,
	/* The inventory must be open to render this. */
	vxen_txt_inventory_only = 8,
	/* Debug text must be enabled to render this. */
	vxen_txt_debug = 16,
	/* Transparency is being modified. */
	vxen_txt_trnsp = 32,
	/* The text buffers should be updated before drawing. */
	vxen_txt_dirty = 128
};

/* Default relative position offset between adjacent text objects. */
#define VX_TEXT_DEFAULT_OFFSET (0.01f)
/* Extra width and height for text background. */
#define VX_TEXT_DEFAULT_BACKGROUND_OFFSET (0.005f)
/* Default amount of time before text starts fading. */
#define VX_TEXT_DEFAULT_FADE_TIME (5.0f)
/* What font size should be considered the 'basis'. */
#define VX_TEXT_DEFAULT_FONT_SIZE (12.0f)

/* Determine whether the given text object is 'empty'. */
#define VX_TEXT_EMPTY(text_obj) (!text_obj.text || !*text_obj.text)

/* Corner positions. */

#define VX_TEXT_LEFTX_CORNER (-1.0f + (VX_TEXT_DEFAULT_BACKGROUND_OFFSET + VX_TEXT_DEFAULT_OFFSET))
#define VX_TEXT_RIGHTX_CORNER (1.0f - (VX_TEXT_DEFAULT_BACKGROUND_OFFSET + VX_TEXT_DEFAULT_OFFSET))

#define VX_TEXT_BOTTOMY_CORNER (-1.0f + (VX_TEXT_DEFAULT_BACKGROUND_OFFSET + VX_TEXT_DEFAULT_OFFSET))
#define VX_TEXT_TOPY_CORNER (1.0f - (VX_TEXT_DEFAULT_BACKGROUND_OFFSET + VX_TEXT_DEFAULT_OFFSET))


/* Data for each character in a text object. */
typedef struct
{
	float x, y, w, h;
	uint32_t char_index, rgba;
} vxtxt_obj_character;
/* Text render object.*/
typedef struct
{
	/* Text to display. */
	char *text;
	/* Text buffer data. */
	vxtxt_obj_character *data;

	/* When to start fading out the text in seconds. */
	float hide_time;
	/* The transparency currently displayed. */
	float displayed_transparency;

	/* XY position of object in relative coordinates (-1.0 to 1.0). */
	float pos_x, pos_y;

	/* Calculated dimensions. */
	float internal_width, internal_height;
	
	/* Font size. Default size and width is defined as a macro. */
	float font_size;
	/* Character spacing to add on top of the default spacing. */
	float extra_char_spacing;
	/* Line spacing to add on top of the default spacing. */
	float extra_line_spacing;

	/* The number of displayable characters present. */
	unsigned short displayed_elements;
	/* Appearance and display settings. */
	unsigned short settings;
} vxtxt_obj;

VX_C_START

/* Initialize a text object with the specified values. */
void vxtxt_obj_init(
	vxtxt_obj *to_init,
	float pos_x, float pos_y,
	char *text,
	int do_copy,
	unsigned int settings,
	float font_size,
	float visibility
);

/* Update the text of the given text object, and marks it as dirty. */
void vxtxt_obj_set_text(vxtxt_obj *to_upd, char *text, int do_copy);
/* Clear the text of the given text object.
   Also sets the text to be invisible. */
void vxtxt_obj_clear_text(vxtxt_obj *to_upd);

/* Functions to handle text changing. */
void vxtxt_obj_text_recalc(vxtxt_obj *to_upd);

/* Get the text object's transparency. */
float vxtxt_obj_get_transparency(vxtxt_obj *text_obj);
/* Determine if the text object is transparent. */
int vxtxt_obj_is_visible(vxtxt_obj *text_obj);
/* Set the static transparency of the text object. */
void vxtxt_obj_set_transparency(vxtxt_obj *text_obj, float transparency);
/* Make the given text object fade out after the given amount of seconds. */
void vxtxt_obj_fade_timer(vxtxt_obj *text_obj, float after_seconds);

/* Move the 'main' text object to be under the 'target' text object by 'offset' normalized coordinates. */
void vxtxt_obj_move_y_relativeto(vxtxt_obj *move_this, vxtxt_obj *relative_to_this, float offset);
/* Move the 'main' text object to be next to (right) the 'target' text object by 'offset' normalized coordinates. */
void vxtxt_obj_move_x_relativeto(vxtxt_obj *move_this, vxtxt_obj *relative_to_this, float offset);

/* Get the width of a text character from the index and size/font multiplier. */
float vxtxt_obj_char_width(int char_ind, float font_multiplier);
/* Get the height of a text character. */
float vxtxt_obj_char_height(vxtxt_obj *text_obj);

/* Calculate and set the width and height of the given text object. */
void vxtxt_obj_update_dims(vxtxt_obj *text_obj);
/* Update the buffers of the text object if needed. */
int vxtxt_obj_update(vxtxt_obj *text_obj);

/* Mark the text object as dirty to update before next draw. */
void vxtxt_obj_dirty(vxtxt_obj *to_mark);

VX_C_END

#endif
