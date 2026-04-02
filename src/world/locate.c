#include "world/locate.h"

#include "directives/dcast.h"

#include <math.h>

#define VX_CHUNK_REGION(qualifier) (VX_REINT_CAST(qualifier vxwld_region *, chunk - chunk->region_index))

void vxwld_global_integral_position(const void *floating_position, wpos *global_position)
{
	const double *fpos = VX_REINT_CAST(const double *, floating_position);
	global_position->x = VX_CAST(pos_type, floor(fpos[0]));
	global_position->y = VX_CAST(pos_type, floor(fpos[1]));
	global_position->z = VX_CAST(pos_type, floor(fpos[2]));
}
void vxwld_global_position_offset(const wpos *global_position, wpos *global_offset)
{
	global_offset->x = VX_GPOS_TO_GOFF(global_position->x, VX_WLD_CHUNK_XBLKS);
	global_offset->y = VX_GPOS_TO_GOFF(global_position->y, VX_WLD_CHUNK_YBLKS);
	global_offset->z = VX_GPOS_TO_GOFF(global_position->z, VX_WLD_CHUNK_ZBLKS);
}


void vxwld_regoff_globpos(const wpos *region_offset, wpos *region_global_position)
{
	region_global_position->x = region_offset->x * VX_WLD_REGION_XBLKS;
	region_global_position->y = region_offset->y * VX_WLD_REGION_YBLKS;
	region_global_position->z = region_offset->z * VX_WLD_REGION_ZBLKS;
}
void vxwld_chunk_globpos(const vxwld_chunk *chunk, wpos *chunk_global_position)
{
	vxwld_regoff_globpos(&VX_CHUNK_REGION(const)->offset, chunk_global_position);
	chunk_global_position->x += VX_XPOS_FROM_INDEX(chunk->region_index, VX_WLD_REGION_YDIM, VX_WLD_REGION_ZDIM) * VX_WLD_CHUNK_XBLKS;
	chunk_global_position->y += VX_YPOS_FROM_INDEX(chunk->region_index, VX_WLD_REGION_YDIM, VX_WLD_REGION_ZDIM) * VX_WLD_CHUNK_YBLKS;
	chunk_global_position->z += VX_ZPOS_FROM_INDEX(chunk->region_index, VX_WLD_REGION_ZDIM) * VX_WLD_CHUNK_ZBLKS;
}
void vxwld_chunk_regpos(const vxwld_chunk *chunk, wpos *chunk_region_position)
{
	chunk_region_position->x = VX_XPOS_FROM_INDEX(chunk->region_index, VX_WLD_REGION_YDIM, VX_WLD_REGION_ZDIM) * VX_WLD_CHUNK_XBLKS;
	chunk_region_position->y = VX_YPOS_FROM_INDEX(chunk->region_index, VX_WLD_REGION_YDIM, VX_WLD_REGION_ZDIM) * VX_WLD_CHUNK_YBLKS;
	chunk_region_position->z = VX_ZPOS_FROM_INDEX(chunk->region_index, VX_WLD_REGION_ZDIM) * VX_WLD_CHUNK_ZBLKS;
}


void vxwld_regoff_from_globoff(const wpos *global_offset, wpos *region_offset)
{
	region_offset->x = VX_GOFF_TO_GOFF(global_offset->x, VX_WLD_REGION_XDIM);
	region_offset->y = VX_GOFF_TO_GOFF(global_offset->y, VX_WLD_REGION_YDIM);
	region_offset->z = VX_GOFF_TO_GOFF(global_offset->z, VX_WLD_REGION_ZDIM);
}
void vxwld_local_chunkoff_from_globoff(const wpos *global_offset, wpos *chunk_offset)
{
	chunk_offset->x = VX_GOFF_TO_LOFF(global_offset->x, VX_WLD_REGION_XDIM);
	chunk_offset->y = VX_GOFF_TO_LOFF(global_offset->y, VX_WLD_REGION_YDIM);
	chunk_offset->z = VX_GOFF_TO_LOFF(global_offset->z, VX_WLD_REGION_ZDIM);
}
void vxwld_globoff_from_chunk(const vxwld_chunk *chunk, wpos *global_offset)
{
	vxwld_chunk_globpos(chunk, global_offset);
	global_offset->x = VX_GPOS_TO_GOFF(global_offset->x, VX_WLD_CHUNK_XBLKS);
	global_offset->y = VX_GPOS_TO_GOFF(global_offset->y, VX_WLD_CHUNK_YBLKS);
	global_offset->z = VX_GPOS_TO_GOFF(global_offset->z, VX_WLD_CHUNK_ZBLKS);
}


vxwld_region *vxwld_region_from_pos(const wpos *global_position)
{
	const wpos region_offset = {
		VX_GPOS_TO_GOFF(global_position->x, VX_WLD_REGION_XBLKS),
		VX_GPOS_TO_GOFF(global_position->y, VX_WLD_REGION_YBLKS),
		VX_GPOS_TO_GOFF(global_position->z, VX_WLD_REGION_ZBLKS)
	};
	return vxwld_get_region(&region_offset);
}
vxwld_chunk *vxwld_chunk_from_pos(const wpos *global_position)
{
	vxwld_region *region = vxwld_region_from_pos(global_position);
	return !region ? VX_NULL : &region->chunks[VX_CHUNK_REGION_INDEX(
		VX_GPOS_TO_LOFF(global_position->x, VX_WLD_CHUNK_XBLKS, VX_WLD_REGION_XDIM),
		VX_GPOS_TO_LOFF(global_position->y, VX_WLD_CHUNK_YBLKS, VX_WLD_REGION_YDIM),
		VX_GPOS_TO_LOFF(global_position->z, VX_WLD_CHUNK_ZBLKS, VX_WLD_REGION_ZDIM)
	)];
}


vxwld_region *vxwld_region_from_offset(const wpos *global_offset)
{
	wpos region_offset;
	vxwld_regoff_from_globoff(global_offset, &region_offset);
	return vxwld_get_region(&region_offset);
}
vxwld_chunk *vxwld_chunk_from_regoff(vxwld_region *region, const wpos *global_offset)
{
	return &region->chunks[VX_CHUNK_REGION_INDEX(
		VX_GOFF_TO_LOFF(global_offset->x, VX_WLD_REGION_XDIM),
		VX_GOFF_TO_LOFF(global_offset->y, VX_WLD_REGION_YDIM),
		VX_GOFF_TO_LOFF(global_offset->z, VX_WLD_REGION_ZDIM)
	)];
}
vxwld_chunk *vxwld_chunk_from_offset(const wpos *global_offset)
{
	vxwld_region *region = vxwld_region_from_offset(global_offset);
	return !region ? VX_NULL : vxwld_chunk_from_regoff(region, global_offset);
}

vxwld_region *vxwld_region_from_chunk(vxwld_chunk *chunk)
{
	return VX_CHUNK_REGION();
}
