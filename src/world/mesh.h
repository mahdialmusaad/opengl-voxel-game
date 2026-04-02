#pragma once
#ifndef SOURCE_WORLD_MESH_VXL_HDR
#define SOURCE_WORLD_MESH_VXL_HDR
/* World mesh handler. */

#include "directives/dextern.h"

#include "world/map.h"

/* Result of a chunk meshing operation. */
typedef struct vxwld_mesh_result
{
	/* Affected chunk. */
	vxwld_chunk *chunk;
	/* Mesh data. */
	vxwld_render *overall_mesh;
	/* Mesh EBO data. */
	uint32_t *overall_ebo;
	/* Mesh data counts. */
	uint32_t mesh_count;
	/* EBO data counts. */
	uint32_t opaque_indices_count, translucent_indices_count;
	uint32_t padding;
} vxwld_mesh_result;

VX_C_START

/* Mesh a chunk into the result data. */
int vxwld_mesh(vxwld_mesh_result *result, vxblk *surrounding_blocks);
/* Mesh any chunks that have been updated. */
size_t vxwld_mesh_changes(VX_NO_ARG);

VX_C_END

#endif
