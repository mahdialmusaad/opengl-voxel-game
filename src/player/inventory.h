#pragma once
#ifndef SOURCE_PLAYER_INVENTORY_VXL_HDR
#define SOURCE_PLAYER_INVENTORY_VXL_HDR
/* Player inventory handler declarations. */

#include "directives/dextern.h"

#include "values/elements.h"

#include "text/text_obj.h"

/* Player inventory GUI state. */
extern struct vxstruct_plr_inventory_gui
{
	/* All text objects relating to the inventory slots. */
	vxtxt_obj inv_text[9 + 36];
	/* Target inventory slot. */
	int selected_slot;
	int free;
} vxplr_gui;

/* Player inventory. */
extern struct vxplr_inv_item {
	vxblk held_id;
	unsigned char count;
	unsigned char other;
} vxplr_inventory[36];

VX_C_START

/* Initialize inventory GUI. */
void vxplr_inv_init(VX_NO_ARG);

/* Toggle inventory visibility. */
void vxplr_inv_toggle(int open);
/* Change the selected invetory slot. */
void vxplr_inv_selected(int slot_index);
/* Change the selected invetory slot from scrolling. */
void vxplr_inv_selected_scroll(double delta_x, double delta_y);

/* Finds the first inventory index with the same ID.
   Optionally takes an argument to specify including those that are 'full'.
   If no match is found, an index of -1 is returned. */
int vxplr_inv_item_slot(unsigned short id, int include_full_slots);
/* Finds the first inventory index that matches the given id and is not 'full', or one that has air.
   On failing finding either, an index of -1 is returned. */
int vxplr_inv_free_slot(unsigned short id);
/* Update the given inventory slot and its corresponding text objects. */
void vxplr_inv_update_slot(int slot_ind, unsigned short id, int count);

/* Update inventory buffers for rendering, and optionally the text sizes.
   Should be done after a window resize or an inventory ID change. */
void vxplr_inv_update(int do_text);

VX_C_END

#endif
