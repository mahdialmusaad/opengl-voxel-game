#pragma once
#ifndef SOURCE_WORLD_MODIFY_VXL_HDR
#define SOURCE_WORLD_MODIFY_VXL_HDR
/* World modifications handler. */

#include "directives/dextern.h"

#include "values/elements.h"

struct wpos;
struct vxwld_chunk;

VX_C_START

/* Store the given chunk's blocks in the smallest format. */
int vxwld_compress_blocks(struct vxwld_chunk *chunk);

/* Indicate that the chunk has been changed and needs an update. */
int vxwld_queue_change(struct vxwld_chunk *chunk);

/* Get the block at the local position in the given chunk. */
vxblk vxwld_local_get(const struct vxwld_chunk *chunk, const struct wpos *local_position);
/* Get the block at the specified position, defaulting to air. */
vxblk vxwld_get(const struct wpos *global_position);
/* Change a block at the specified position. */
int vxwld_set(const struct wpos *global_position, vxblk set_id);

VX_C_END

#endif
