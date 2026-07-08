#include "player/edit.h"
#include "player/inventory.h"
#include "player/movement.h"
#include "player/raycast.h"
#include "events/events.h"
#include "player/camera.h"

#include "values/elements.h"

#include "graphics/glfw.h"

#include "world/modify.h"
#include "world/locate.h"
#include "world/map.h"

static int ray_queue(void)
{
	if (vxwld_tomesh_queue.size) return 0;
	vxplr_ray_cast_local(&vxplr_inst.pos, &vxplr_cam.front);
	return 1;
}

static void vxplr_edit_updside(const wpos *curoff, int side)
{
	wpos goff;
	wpos_add(&goff, curoff, vxelm_dirs + side);
	vxwld_queue_change(vxwld_chunk_from_offset(&goff));
}

/* Update any corner chunks that would have been affected by a change in an adjacent chunk. */
static void vxplr_edit_updaround(const wpos *p)
{
	wpos localpos = { VX_GPOS_TO_LPOS(p->x, VX_WLD_CHUNK_XBLKS), VX_GPOS_TO_LPOS(p->y, VX_WLD_CHUNK_YBLKS), VX_GPOS_TO_LPOS(p->z, VX_WLD_CHUNK_ZBLKS) }, curoff;
	vxwld_global_position_offset(p, &curoff);

	if (localpos.x == VX_WLD_CHUNK_XBLKS - 1) vxplr_edit_updside(&curoff, wdir_right);
	else if (localpos.x == 0) vxplr_edit_updside(&curoff, wdir_left);
	if (localpos.y == VX_WLD_CHUNK_YBLKS - 1) vxplr_edit_updside(&curoff, wdir_top);
	else if (localpos.y == 0) vxplr_edit_updside(&curoff, wdir_down);
	if (localpos.z == VX_WLD_CHUNK_ZBLKS - 1) vxplr_edit_updside(&curoff, wdir_front);
	else if (localpos.z == 0) vxplr_edit_updside(&curoff, wdir_back);
}

void vxplr_edit_general(int button, int action)
{
	if (action != GLFW_PRESS) return;
	if (!vxelm_elements[vxplr_ray.slctd_type].solid) return;

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		vxblk at = vxwld_get(&vxplr_ray.slctd_pos);
		vxwld_set(&vxplr_ray.slctd_pos, 0);
		vxplr_edit_updaround(&vxplr_ray.slctd_pos);
		int tochange = vxplr_inv_free_slot(at);
		if (tochange == -1) return;
		vxplr_inv_update_slot(tochange, at, vxplr_inventory[tochange].count + 1);
	} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		wpos pos = { vxelm_dirs[vxplr_ray.slctd_side].x, vxelm_dirs[vxplr_ray.slctd_side].y, vxelm_dirs[vxplr_ray.slctd_side].z };
		wpos_add(&pos, &vxplr_ray.slctd_pos, &pos);
		vxwld_set(&pos, vxplr_inventory[vxplr_gui.selected_slot].held_id);
		vxplr_edit_updaround(&pos);
		vxplr_inv_update_slot(vxplr_gui.selected_slot, vxplr_inventory[vxplr_gui.selected_slot].held_id, vxplr_inventory[vxplr_gui.selected_slot].count - 1);
	}

	vxevent_add(ray_queue);
}
