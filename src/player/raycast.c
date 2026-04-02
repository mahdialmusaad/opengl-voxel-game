#include "player/raycast.h"

#include "directives/dmath.h"

#include "values/elements.h"
#include "graphics/glctx.h"

#include "text/text_obj.h"

#include "world/modify.h"
#include "world/locate.h"

#include "vector/vec3.h"
#include "vector/wpos.h"

#include "io/format.h"

#include <math.h>

struct vxstruct_ray_state_obj vxplr_ray;
static vxtxt_obj vxplr_ray_txt;

static int vxplr_ray_check_visible(void *draw)
{
	(void)(draw);
	return vxelm_elements[vxplr_ray.slctd_type].solid && !vxplr_ray.ray_inside;
}

void vxplr_ray_init(VX_NO_ARG)
{
	const float offset = 0.001f, front1 = 1.0f + offset, front0 =  offset, small1 = 1.0f - offset, small0 = -offset;
	const float outline_shape_vertices[] = {
		small0, front1, small0,  small1, front1, small0,  small1, front1, small1,  small0, front1, small1, /* Top face. */
		small1, front0, small0,  small0, front0, small0,  small0, front0, small1,  small1, front0, small1, /* Bottom face. */
		front1, small0, small1,  front1, small0, small0,  front1, small1, small0,  front1, small1, small1, /* Right face. */
		front0, small0, small1,  front0, small0, small0,  front0, small1, small0,  front0, small1, small1, /* Left face. */
		small0, small1, front1,  small0, small0, front1,  small1, small0, front1,  small1, small1, front1, /* Front face. */
		small0, small1, front0,  small0, small0, front0,  small1, small0, front0,  small1, small1, front0, /* Back face. */
	};
	const uint8_t outline_shape_indices[] = {
		0,  1,  1,  2,  2,  3,  3,  0,	/* Top face. */
		4,  5,  5,  6,  6,  7,  7,  4,	/* Bottom face. */
		8,  9,  9,  10, 10, 11, 11, 8,	/* Right face. */
		12, 13, 13, 14, 14, 15, 15, 12,	/* Left face. */
		16, 17, 17, 18, 18, 19, 19, 16,	/* Front face. */
		20, 21, 21, 22, 22, 23, 23, 20	/* Back face. */
	};

	/* Setup the block outline rendering context. */
	vxctx_init(&vxsd_shaders.outline,
		VX_CTX_VBO(outline_shape_vertices), VX_CTX_NVBO, VX_CTX_EBO(outline_shape_indices),
		VX_NULL, vxen_ctxorder_outline, vxen_ctxmode_lines,
		VX_NULL, 0, vxplr_ray_check_visible
	);
	vxtxt_obj_init(
		&vxplr_ray_txt,
		0.0f, 0.0f,
		VX_NULL, 1,
		vxen_txt_background | vxen_txt_shadow | vxen_txt_debug,
		VX_TEXT_DEFAULT_FONT_SIZE, 1
	);
}


int vxplr_ray_cast(const dvec3 *start_pos, const dvec3 *direction, wpos *target_pos, int *target_normal, unsigned short *target_type)
{
	/* DDA algorithm for raycasting. */
	vxwld_global_integral_position(start_pos, target_pos);
	wpos previous_position = *target_pos;
	
	const ivec3 step = { VX_NEG_ONE_SIGN(direction->x), VX_NEG_ONE_SIGN(direction->y), VX_NEG_ONE_SIGN(direction->z) };
	const dvec3 ray_delta = { fabs(1.0 / direction->x), fabs(1.0 / direction->y), fabs(1.0 / direction->z) };
	dvec3 side_dist = {
		(VX_CAST(double, target_pos->x * step.x) + VX_ZERO_ONE_SIGN(direction->x) + (-start_pos->x * step.x)) * ray_delta.x,
		(VX_CAST(double, target_pos->y * step.y) + VX_ZERO_ONE_SIGN(direction->y) + (-start_pos->y * step.y)) * ray_delta.y,
		(VX_CAST(double, target_pos->z * step.z) + VX_ZERO_ONE_SIGN(direction->z) + (-start_pos->z * step.z)) * ray_delta.z
	};

	enum vxen_movement_axis_def { vxen_shortest_x, vxen_shortest_y, vxen_shortest_z };

	/* Advance in the shortest axis each time. */
	int ray_advances = 0;
	for (; ray_advances < VX_RAY_MAX_ITERATIONS; ++ray_advances) {
		*target_type = vxwld_get(target_pos);
		if (vxelm_elements[*target_type].solid) break;
		previous_position = *target_pos;

		const int shortest_axis = side_dist.x < side_dist.y ?
		  (side_dist.x < side_dist.z ? vxen_shortest_x : vxen_shortest_z) :
		  (side_dist.y < side_dist.z ? vxen_shortest_y : vxen_shortest_z);
		
		if (shortest_axis == vxen_shortest_x) {
			target_pos->x += step.x;
			side_dist.x += ray_delta.x;
		} else if (shortest_axis == vxen_shortest_y) {
			target_pos->y += step.y;
			side_dist.y += ray_delta.y;
		} else {
			target_pos->z += step.z;
			side_dist.z += ray_delta.z;
		}
	}

	wpos last_pos_difference;
	wpos_sub(&last_pos_difference, &previous_position, target_pos);

	/* Determine world direction vector index of the last movement for block placing. */
	for (*target_normal = 0;
		wpos_neq(&last_pos_difference, vxelm_dirs + *target_normal) && *target_normal < 6;
		++(*target_normal));

	return ray_advances;
}

void vxplr_ray_cast_local(const dvec3 *start_pos, const dvec3 *direction)
{
	const unsigned short original_type = vxplr_ray.slctd_type;
	const wpos original_pos = vxplr_ray.slctd_pos;

	/* Cast a ray for the player's selected block, and determine whether the ray text needs to be updated. */
	const int ray_advances = vxplr_ray_cast(start_pos, direction, &vxplr_ray.slctd_pos, &vxplr_ray.slctd_side, &vxplr_ray.slctd_type);
	const short int inside_changed = vxplr_ray.ray_inside != (ray_advances == 0);
	vxplr_ray.ray_inside ^= inside_changed;

	/* Do not update if there is no change in the selected block. */
	if (!inside_changed && vxplr_ray.slctd_type == original_type && wpos_eq(&vxplr_ray.slctd_pos, &original_pos)) return;
	vxplr_ray_update();
}


void vxplr_ray_update(VX_NO_ARG)
{
	const vxelm_attribs *block_data = vxelm_elements + vxplr_ray.slctd_type;

	wpos block_offset;
	vxwld_global_position_offset(&vxplr_ray.slctd_pos, &block_offset);

	char *ray_info = vxfmt_text(
		"Selected%s:\n"
		 VXFMTPOS " " VXFMTPOS " " VXFMTPOS " (" VXFMTPOS " " VXFMTPOS " " VXFMTPOS ")\n"
		"\"%s\" (%d)\n"
		"Strength: %d\nSolid: %d\nTransparency: %d\n"
		"Textures: %u %u %u %u %u %u",
		vxplr_ray.ray_inside ? " (Inside)" : "",
		vxplr_ray.slctd_pos.x, vxplr_ray.slctd_pos.y, vxplr_ray.slctd_pos.z,
		block_offset.x, block_offset.y, block_offset.z,
		block_data->name, VX_CAST(int, vxplr_ray.slctd_type),
		block_data->strength, block_data->solid, block_data->transparency,
		block_data->textures[0], block_data->textures[1], block_data->textures[2],
		block_data->textures[3], block_data->textures[4], block_data->textures[5]
	);
	if (!ray_info) return;

	vxtxt_obj_set_text(&vxplr_ray_txt, ray_info, 0);
	vxplr_ray_txt.pos_x = 1.0f - vxplr_ray_txt.internal_width;
	vxplr_ray_txt.pos_y = VX_TEXT_TOPY_CORNER;
}
