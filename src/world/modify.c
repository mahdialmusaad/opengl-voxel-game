#include "world/modify.h"
#include "world/locate.h"

#include "directives/dcast.h"

#include <stdlib.h>

int vxwld_compress_blocks(vxwld_chunk *chunk)
{
	(void)(chunk);
	return 1;
}

int vxwld_queue_change(vxwld_chunk *chunk)
{
	if (chunk->is_queued) return 0;
	chunk->is_queued = 1u;
	return vxdy_array_add(&vxwld_tomesh_queue, &chunk) != VX_NULL;
}

vxblk vxwld_get(const wpos *global_position)
{
	const vxwld_chunk *chunk = vxwld_chunk_from_pos(global_position);
	return !chunk || !chunk->blocks.data ? 0 : chunk->blocks.data[VX_POS_AS_INDEX(global_position)];
}

int vxwld_set(const wpos *global_position, vxblk set_id)
{
	vxwld_chunk *chunk = vxwld_chunk_from_pos(global_position);
	if (!chunk) return 0;
	if (!chunk->blocks.data && !set_id) return 1;
	
	if (!chunk->blocks.data) {
		chunk->blocks.data = VX_CAST(vxblk *, calloc(VX_WLD_CHUNK_BLOCKS, sizeof *chunk->blocks.data));
		if (!chunk->blocks.data) return 0;
	}

	chunk->blocks.data[VX_POS_AS_INDEX(global_position)] = set_id;
	vxwld_compress_blocks(chunk);

	return vxwld_queue_change(chunk);
}
