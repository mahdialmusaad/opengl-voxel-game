#include "text/text_mgr.h"
#include "text/text_obj.h"

#include "directives/dcast.h"
#include "directives/dfree.h"
#include "directives/dword.h"
#include "directives/dsets.h"

#include "shaders/programs.h"
#include "shaders/textures.h"

#include "graphics/glctx.h"

#include "values/state.h"

#include <string.h>
#include <stdlib.h>

struct vxtxt_mgr_obj vxtxt_manager = {
	VX_NULL,
	VX_TEXT_DEFAULT_FONT_SIZE_WIDTH,
	VX_TEXT_DEFAULT_FONT_SIZE_HEIGHT,
	60, 11, /* Chat limits. */
	VX_DYARRAY_INIT(sizeof(vxtxt_obj *), 1),
	{
	/* !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  */
	   1, 3, 6, 5, 9, 6, 1, 2, 2, 5, 5, 2, 5, 1, 3,
	/* 0  1  2  3  4  5  6  7  8  9  */
	   5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	/* :  ;  <  =  >  ?  @  */
	   1, 2, 4, 5, 4, 5, 6,
	/* A  B  C  D  E  F  G  H  I  J  K  L  M  */
	   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	/* N  O  P  Q  R  S  T  U  V  W  X  Y  Z  */
	   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	/* [  \  ]  ^  _  `   */
	   2, 3, 2, 5, 5, 2,
	/* a  b  c  d  e  f  g  h  i  j  k  l  m  */
	   4, 4, 4, 4, 4, 4, 4, 4, 1, 3, 4, 3, 5,
	/* n  o  p  q  r  s  t  u  v  w  x  y  z  */
	   4, 4, 4, 4, 4, 4, 3, 4, 5, 5, 5, 4, 4,
	/* {  |  }  ~  */
	   3, 1, 3, 6,
	/* Custom background character. */
	   1
	},
	0, 0 /* Update checks. */
};

static int vxtxt_mgr_update(void *draw)
{
	if (!vxtg_toggles.show_any_gui) return 0;
	if (!vxtxt_manager.any_update) return 1;

	const int is_inventory_text = vxctx_equal_draws(draw, vxtxt_manager.inv_text_buffer);
	size_t total_bytes = 0u;
	int any_update = 0;

	for (size_t i = 0u; i < vxtxt_manager.text_obj_array.size; ++i) {
		vxtxt_obj *text_obj = VX_CAST(vxtxt_obj **, vxtxt_manager.text_obj_array.data)[i];
		if ((!!(text_obj->settings & vxen_txt_inventory_only)) != is_inventory_text) continue;
		if (vxtxt_obj_update(text_obj)) any_update = 1;
		total_bytes += sizeof *text_obj->data * text_obj->displayed_elements; 
	}

	if (!any_update) return 1;
	void *accumulated = malloc(total_bytes);
	if (!accumulated) return 0;

	for (size_t i = 0u, reached_bytes = 0u; i < vxtxt_manager.text_obj_array.size; ++i) {
		vxtxt_obj *text_obj = VX_CAST(vxtxt_obj **, vxtxt_manager.text_obj_array.data)[i];
		if (((text_obj->settings & vxen_txt_inventory_only) != 0) != is_inventory_text) continue;
		if (!text_obj->displayed_elements) continue;

		const size_t data_bytes = sizeof(vxtxt_obj_character) * VX_CAST(size_t, text_obj->displayed_elements);
		memcpy(VX_CAST(unsigned char *, accumulated) + reached_bytes, text_obj->data, data_bytes);
		reached_bytes += data_bytes;
	}

	vxtxt_manager.any_update = !is_inventory_text;
	vxctx_update_buffer(draw, accumulated, 0u, total_bytes);
	VX_FREE(accumulated);

	return 1;
}

void vxtxt_mgr_init(VX_NO_ARG)
{
	/* Unique characters * 2 (for start and end position) + 1 (for end of image). */
	#define VX_TEXT_MGR_TEXCHARS_COUNT ((sizeof vxtxt_manager.char_sizes * 2u) + 1u)
	
	float text_uniform_pos[VX_TEXT_MGR_TEXCHARS_COUNT];
	text_uniform_pos[VX_TEXT_MGR_TEXCHARS_COUNT - 1u] = 1.0f;
	
	/* Each character is separated by a pixel on either side in the image texture, skip 2 pixels each character. */
	float pixel_width = 1.0f / VX_CAST(float, vxtex_textures.text.width), cur_img_offset = pixel_width;
	for (size_t i = 0u, n = 0u; i < sizeof vxtxt_manager.char_sizes; ++i) {
		const float char_sz = VX_CAST(float, vxtxt_manager.char_sizes[i]);
		text_uniform_pos[n++] = cur_img_offset;
		text_uniform_pos[n++] = cur_img_offset + (pixel_width * char_sz);
		cur_img_offset += pixel_width * (char_sz + 2.0f);
	}

	vxsd_set_floats(&vxsd_shaders.text, vxsd_get_location(&vxsd_shaders.text, "texture_positions"), text_uniform_pos, VX_TEXT_MGR_TEXCHARS_COUNT);
	#undef VX_TEXT_MGR_TEXCHARS_COUNT

	if (vxtxt_manager.inited) {
		vxtxt_mgr_all_dirty();
		return;
	}
	vxtxt_manager.inited = 1;

	const float quad_verts[4][2] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };

	/* Initialize the text rendering context. */
	vxctx_init(&vxsd_shaders.text,
		VX_CTX_VBO(quad_verts), VX_CTX_BVBO(VX_NULL, sizeof(vxtxt_obj_character)), VX_CTX_NEBO,
		VX_NULL, vxen_ctxorder_text, vxen_ctxmode_tristrips,
		VX_NULL, 0, vxtxt_mgr_update
	);
	vxctx_init(&vxsd_shaders.text,
		VX_CTX_VBO(quad_verts), VX_CTX_BVBO(VX_NULL, sizeof(vxtxt_obj_character)), VX_CTX_NEBO,
		&vxtxt_manager.inv_text_buffer, vxen_ctxorder_text, vxen_ctxmode_tristrips,
		VX_NULL, 0, vxtxt_mgr_update
	);
}

void vxtxt_mgr_all_dirty(VX_NO_ARG)
{
	for (size_t i = 0u; i < vxtxt_manager.text_obj_array.size; ++i) {
		vxtxt_obj *text_ptr = VX_CAST(vxtxt_obj **, vxtxt_manager.text_obj_array.data)[i];
		vxtxt_obj_text_recalc(text_ptr);
	}
}

void vxtxt_mgr_destroy(VX_NO_ARG)
{
	VX_TEXTMGR_LOG(1, "%zu texts bye", vxtxt_manager.text_obj_array.size);
	for (size_t i = 0u; i < vxtxt_manager.text_obj_array.size; ++i) {
		vxtxt_obj *text_ptr = VX_CAST(vxtxt_obj **, vxtxt_manager.text_obj_array.data)[i];
		if (text_ptr->text) VX_FREE(text_ptr->text);
		if (text_ptr->data) VX_FREE(text_ptr->data);
	}
	vxdy_array_free(&vxtxt_manager.text_obj_array);
}
