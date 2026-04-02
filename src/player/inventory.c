#include "player/inventory.h"

#include "directives/dcast.h"
#include "directives/dword.h"
#include "directives/dfree.h"

#include "text/text_obj.h"

#include "graphics/glctx.h"
#include "graphics/glfw.h"

#include "values/elements.h"
#include "values/state.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Maximum number of 'items' that can be held in a single inventory 'slot'. */
#define VX_MAX_SLOT_COUNT (64)
/* Maximum number of inventory elements rendered at once. */
#define VX_INV_RENDER_MAX (9 * 5 * 2 + 1)

struct vxstruct_plr_inventory_gui vxplr_gui;
struct vxplr_inv_item vxplr_inventory[36];

typedef struct
{
	/* X, Y, width, height. */
	float dims[4];
	uint32_t texture_id;
} vxplr_gui_elem;

static void *vxplr_gui_buffer;

void vxplr_inv_init(VX_NO_ARG)
{
	memset(&vxplr_inventory, 0, sizeof vxplr_inventory);

	const float gui_quad_vertices[] = {
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 1.0f
	};

	/* Initialize player GUI rendering context. */
	vxctx_init(&vxsd_shaders.inventory,
		VX_CTX_VBO(gui_quad_vertices), VX_CTX_BVBO(VX_NULL, sizeof(vxplr_gui_elem) * VX_INV_RENDER_MAX), VX_CTX_NEBO,
		&vxplr_gui_buffer, vxen_ctxorder_inventory, vxen_ctxmode_tristrips,
		&vxtg_toggles.show_any_gui, vxen_ctxcond_uchar, VX_NULL
	);

	for (size_t i = 0u; i < sizeof vxplr_gui.inv_text / sizeof *vxplr_gui.inv_text; ++i) {
		vxtxt_obj_init(
			vxplr_gui.inv_text + i,
			0.0f, VX_TEXT_BOTTOMY_CORNER,
			VX_NULL, 1,
			((i < 9u) * vxen_txt_inventory_only) | vxen_txt_shadow,
			VX_TEXT_DEFAULT_FONT_SIZE, 0.0f
		);
	}
}


void vxplr_inv_toggle(int open)
{
	vxtg_toggles.inventory_open = open == 1;
	if (open) {
		glfwSetCursorPos(vxstate_vals.window_ptr, VX_CAST(double, vxstate_vals.window_width) * 0.5, VX_CAST(double, vxstate_vals.window_height) * 0.5);
		glfwSetInputMode(vxstate_vals.window_ptr, GLFW_CURSOR, vxtg_toggles.inventory_open ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	
	vxtg_toggles.window_focus_changed = 1;
	vxtg_toggles.show_any_gui = 1;

	vxplr_inv_update(1);
}

void vxplr_inv_selected(int slot_index)
{
	if (slot_index < 0) slot_index = 8;
	else if (slot_index >= 9) slot_index = 0;
	vxplr_gui.selected_slot = slot_index;
	vxplr_inv_update(0);
}
void vxplr_inv_selected_scroll(double delta_x, double delta_y)
{
	(void)(delta_x);
	vxplr_inv_selected(vxplr_gui.selected_slot + (delta_y > 0.0 ? 1 : -1));
}


int vxplr_inv_item_slot(unsigned char id, int include_full_slots)
{
	const struct vxplr_inv_item *end_slot = vxplr_inventory + (sizeof vxplr_inventory / sizeof *vxplr_inventory);
	for (const struct vxplr_inv_item *cur_slot = vxplr_inventory; cur_slot != end_slot; ++cur_slot) {
		if (cur_slot->held_id != id) continue;
		if (!include_full_slots && cur_slot->count >= VX_MAX_SLOT_COUNT) continue;
		return VX_CAST(int, cur_slot - vxplr_inventory);
	}
	return -1;
}

int vxplr_inv_free_slot(unsigned char id)
{
	const int matching_slot_ind = vxplr_inv_item_slot(id, 0);
	if (matching_slot_ind != -1) return matching_slot_ind;
	return vxplr_inv_item_slot(0, 0);
}

void vxplr_inv_update_slot(int slot_ind, unsigned char id, int count)
{
	if (slot_ind == -1) return;
	if (count > VX_MAX_SLOT_COUNT) return;
	
	struct vxplr_inv_item *slot = vxplr_inventory + slot_ind;
	vxtxt_obj *inv_text = vxplr_gui.inv_text + slot_ind;
	
	int needs_inv_upd = id != slot->held_id;

	if (count == 0 || !id) {
		vxtxt_obj_clear_text(inv_text);
		needs_inv_upd |= slot->count != 0;
		slot->held_id = 0;
		slot->count = 0;
	} else {
		slot->held_id = id;
		slot->count = VX_CAST(unsigned char, count);
		char slot_str[3] = { VX_CAST(char, (count / 10) % 10), VX_CAST(char, count % 10), 0 };
		vxtxt_obj_set_text(inv_text, slot_str, 0);
		vxtxt_obj_set_transparency(inv_text, 1.0f);
	}

	if (needs_inv_upd) vxplr_inv_update(0);
}


/* Used to determine specific positioning and sizes of inventory elements. */
enum {
	vxen_inv_hotbar_slot,
	vxen_inv_inventory_slot,
	vxen_inv_def_size,
	vxen_inv_block_size
};

/* Insert the position or size of an inventory slot from the given identifiers into the array. */
static void vxplr_gui_get_dims(float *result, int id, int is_block_tex)
{
	const float slot_width = 0.1f, slot_height = slot_width * vxstate_vals.aspect;
	const float inventory_hotbar_ypos = -0.4f;
	const float inner_inventory_yoffset = slot_width * 0.1f;
	const float slots_start_xpos = -0.5f * slot_width * 9.0f;

	const int inventory_row = (id - 9) / 9;
	const float slot_ypos = id < 9 ? -1.0f : (id >= 18 ? (inventory_hotbar_ypos + inner_inventory_yoffset + (slot_height * VX_CAST(float,inventory_row))) : inventory_hotbar_ypos);
	const float slot_xpos = slots_start_xpos + (VX_CAST(float, id % 9) * slot_width);

	if (is_block_tex) {
		const float slot_blocktex_width = slot_width * 0.75f, slot_blocktex_height = slot_height * 0.75f;
		const float blocktex_yoffset = (slot_height - slot_blocktex_height) * 0.5f, blocktex_xoffset = ((slot_width - slot_blocktex_width) * 0.5f);
		result[0] = slot_xpos + blocktex_xoffset;
		result[1] = slot_ypos + blocktex_yoffset;
		result[2] = slot_blocktex_width;
		result[3] = slot_blocktex_height;
	} else {
		result[0] = slot_xpos;
		result[1] = slot_ypos;
		result[2] = slot_width;
		result[3] = slot_height;
	}
}

static void vxplr_inv_update_text(VX_NO_ARG)
{
	const float text_offset_x = 0.02f, text_offset_y = text_offset_x * vxstate_vals.aspect;

	for (size_t i = 0u; i < sizeof vxplr_gui.inv_text / sizeof *vxplr_gui.inv_text; ++i) {
		vxtxt_obj *cur_slot_txt = vxplr_gui.inv_text + i;
		float res_pos[4];
		vxplr_gui_get_dims(res_pos, VX_CAST(int, i), 0);

		cur_slot_txt->pos_x = res_pos[0] + text_offset_x;
		cur_slot_txt->pos_y = res_pos[1] + text_offset_y;

		vxtxt_obj_dirty(cur_slot_txt);
	}
}

/* Used internally for updating an inventory instance's data. */
static void vxplr_gui_set_inst(vxplr_gui_elem **buffer_data, int id, int is_block_tex, int texture_id)
{
	vxplr_gui_elem *cur_buf = *buffer_data;
	vxplr_gui_get_dims(cur_buf->dims, id, is_block_tex);
	cur_buf->texture_id = VX_CAST(uint32_t, is_block_tex + (texture_id << 1));
	*buffer_data = ++cur_buf;

	if (is_block_tex) return;
	const unsigned int slot_id = vxplr_inventory[id >= 9 ? id - 9 : id].held_id;
	if (!slot_id) return;

	vxplr_gui_set_inst(&cur_buf, id, 1, vxelm_elements[slot_id].textures[wdir_top]);
	*buffer_data = ++cur_buf;
}


void vxplr_inv_update(int do_text)
{
	if (do_text) vxplr_inv_update_text();

	/* Each texture index (when data is of an inventory texture rather than a block texture)
	   is used to index into an array that determines the texture positions in the vertex shader. */
	enum { vxen_tex_background = 0, vxen_tex_unequipped = 1, vxen_tex_equipped = 2, vxen_tex_crosshair = 3 };

	vxplr_gui_elem *vxplr_gui_data = VX_CAST(vxplr_gui_elem *, malloc(sizeof *vxplr_gui_data * VX_INV_RENDER_MAX)), *hotbar_data = vxplr_gui_data;
	if (!vxplr_gui_data) return;

	if (vxtg_toggles.inventory_open) {
		const vxplr_gui_elem background = { { -1.0f, -1.0f, 2.0f, 2.0f }, vxen_tex_background << 1 };
		*hotbar_data++ = background;
	}

	/* Hotbar slots calculation. */
	for (int id = 0; id < 9; ++id) vxplr_gui_set_inst(&hotbar_data, id, 0, vxen_tex_unequipped + (id == vxplr_gui.selected_slot));

	if (!vxtg_toggles.inventory_open) {
		const float ch_height = 0.025f, ch_width = ch_height / vxstate_vals.aspect;
		const vxplr_gui_elem crosshair = { { -ch_width, -ch_height, ch_width, ch_height }, vxen_tex_crosshair * 2 };
		*hotbar_data++ = crosshair;
		goto update_immediate;
	}

	/* Calculate inventory slots. */
	for (int id = 9; id < 45; ++id) vxplr_gui_set_inst(&hotbar_data, id, 0, vxen_tex_unequipped + ((id - 9) == vxplr_gui.selected_slot));
update_immediate:
	/* Buffer inventory data for use in the shader. */
	vxctx_update_buffer(vxplr_gui_buffer, vxplr_gui_data, 0u, sizeof(vxplr_gui_elem) * VX_CAST(size_t, hotbar_data - vxplr_gui_data));
	VX_FREE(vxplr_gui_data);
}
