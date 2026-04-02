#pragma once
#ifndef SOURCE_WORLD_LOCATE_VXL_HDR
#define SOURCE_WORLD_LOCATE_VXL_HDR
/* General positioning functions. */

#include "directives/dextern.h"

#include "world/map.h"

/* Positioning macros. */

/* Example: Get the chunk 3D index in a region using the global position. */
#define VX_GPOS_TO_LOFF(pos, section_dim, max) (((((pos) + ((pos) < 0)) / (section_dim)) % (max)) + (((pos) < 0) * ((max) - 1)))
/* Example: Get the chunk global offset from the global position. */
#define VX_GPOS_TO_GOFF(pos, section_dim) (((pos) - (((pos) < 0) * ((section_dim) - 1))) / (section_dim))
/* Example: Get block position in a chunk from the global position. */
#define VX_GPOS_TO_LPOS(pos, section_dim) (((pos) % (section_dim) + (section_dim)) % (section_dim))
/* Example: Get the local chunk offset in a region from the global offset. */
#define VX_GOFF_TO_LOFF(off, max) ((((off) + ((off) < 0)) % (max)) + (((off) < 0) * ((max) - 1)))
/* Example: Get region offset from a global offset. */
#define VX_GOFF_TO_GOFF(off, section_dim) VX_GPOS_TO_GOFF(off, section_dim)

/* Indexing macros. */

#define VX_INDEX_FROM_XYZ(x, y, z, ydim, zdim) (((x) * (ydim) * (zdim)) + ((y) * (zdim)) + (z))
#define VX_XPOS_FROM_INDEX(index, ydim, zdim) ((index) / ((ydim) * (zdim)))
#define VX_YPOS_FROM_INDEX(index, ydim, zdim) (((index) / (zdim)) % (ydim))
#define VX_ZPOS_FROM_INDEX(index, zdim) ((index) % (zdim))

/* Local 3D position as block index. */
#define VX_POS_AS_INDEX_SEP(x, y, z) VX_CAST(size_t, VX_INDEX_FROM_XYZ(\
	VX_GPOS_TO_LPOS(x, VX_WLD_CHUNK_XBLKS),\
	VX_GPOS_TO_LPOS(y, VX_WLD_CHUNK_YBLKS),\
	VX_GPOS_TO_LPOS(z, VX_WLD_CHUNK_ZBLKS),\
	VX_WLD_CHUNK_YBLKS, VX_WLD_CHUNK_ZBLKS\
))
#define VX_POS_AS_INDEX(vec) VX_POS_AS_INDEX_SEP(vec->x, vec->y, vec->z)

#define VX_CHUNK_REGION_INDEX(x, y, z) (((x) * VX_WLD_REGION_YDIM * VX_WLD_REGION_ZDIM) + ((y) * VX_WLD_REGION_ZDIM) + (z))

VX_C_START

/* Get the global position from a double-precision floating-point position vector. */
void vxwld_global_integral_position(const void *floating_position, wpos *global_position);
/* Get the global offset from the global position. */
void vxwld_global_position_offset(const wpos *global_position, wpos *global_offset);

/* Get the region global position from the region offset. */
void vxwld_regoff_globpos(const wpos *region_offset, wpos *region_global_position);
/* Get the chunk's global position. */
void vxwld_chunk_globpos(const vxwld_chunk *chunk, wpos *chunk_global_position);
/* Get the chunk's region position. */
void vxwld_chunk_regpos(const vxwld_chunk *chunk, wpos *chunk_region_position);

/* Get the offset of the region that contains the given global offset. */
void vxwld_regoff_from_globoff(const wpos *global_offset, wpos *region_offset);
/* Get the local (region 3D index) offset of the chunk located at the given global offset. */
void vxwld_local_chunkoff_from_globoff(const wpos *global_offset, wpos *chunk_offset);
/* Get the global offset of the given chunk. */
void vxwld_globoff_from_chunk(const vxwld_chunk *chunk, wpos *global_offset);

/* Get the region from the global position. */
vxwld_region *vxwld_region_from_pos(const wpos *global_position);
/* Get the chunk from the global position. */
vxwld_chunk *vxwld_chunk_from_pos(const wpos *global_position);

/* Get the region from the global chunk offset. */
vxwld_region *vxwld_region_from_offset(const wpos *global_offset);
/* Get the chunk from the global offset and region. */
vxwld_chunk *vxwld_chunk_from_regoff(vxwld_region *region, const wpos *global_offset);
/* Get the chunk from the global offset. */
vxwld_chunk *vxwld_chunk_from_offset(const wpos *global_offset);

/* Get the enclosing region of the given chunk. */
vxwld_region *vxwld_region_from_chunk(vxwld_chunk *chunk);

VX_C_END

#endif
