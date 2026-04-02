#pragma once
#ifndef SOURCE_WORLD_MAP_VXL_HDR
#define SOURCE_WORLD_MAP_VXL_HDR
/* World map values. */

#include "directives/dextern.h"
#include "directives/dfast.h"
#include "directives/dcast.h"

#include "values/elements.h"

#include "utils/dyarray.h"

#include "vector/wpos.h"

#include "world/wsets.h"

#define VX_WLD_DEBUG 0

/* Number of blocks in a chunk. */
#define VX_WLD_CHUNK_BLOCKS (VX_WLD_CHUNK_XBLKS * VX_WLD_CHUNK_YBLKS * VX_WLD_CHUNK_ZBLKS)
/* Number of chunks in a region. */
#define VX_WLD_REGION_ALLDIM (VX_WLD_REGION_XDIM * VX_WLD_REGION_YDIM * VX_WLD_REGION_ZDIM)

/* Number of block in a region in the X axis. */
#define VX_WLD_REGION_XBLKS (VX_WLD_REGION_XDIM * VX_WLD_CHUNK_XBLKS)
/* Number of block in a region in the Y axis. */
#define VX_WLD_REGION_YBLKS (VX_WLD_REGION_YDIM * VX_WLD_CHUNK_YBLKS)
/* Number of block in a region in the Z axis. */
#define VX_WLD_REGION_ZBLKS (VX_WLD_REGION_ZDIM * VX_WLD_CHUNK_ZBLKS)

/* Render data per vertex. */
typedef struct vxwld_render
{
	/* Region 3D position. Each axis uses 20 bits. */
	uint32_t packed_pos_xy, packed_pos_yz : 28;
	/* Exposure to world lighting. */
	uint32_t light_world : 4;
	/* Individual light colours. */
	uint32_t light_r : 4, light_g : 4, light_b : 4;
	/* Texture coordinates. */
	uint32_t texture_v_id : 16, texture_u : 4;
} vxwld_render;

/* Type used for EBO index. */
typedef uint32_t vxwld_ebo;

/* Chunk blocks type. */
typedef struct { vxblk *data; } vxwld_blocks;

/* Data on a section of blocks. */
typedef struct vxwld_chunk
{
	/* Blocks list. */
	vxwld_blocks blocks;
	/* How many indices there are, for both opaque and translucent triangles. */
	uint32_t opaque_indices_count, translucent_indices_count;
	/* Drawing buffers. */
	unsigned int vao, vbo, ebo;
	/* 1D index into the enclosing region's chunk array. */
	uint32_t region_index : 30;
	/* Whether this chunk has been affected by generation. */
	uint32_t is_decorated : 1;
	/* Whether this chunk is going to be meshed. */
	uint32_t is_queued : 1;
} vxwld_chunk;

/* A region of chunks. */
typedef struct vxwld_region
{
	/* 3D grid of chunks belonging to this region. */
	vxwld_chunk chunks[VX_WLD_REGION_ALLDIM];
	/* The global offset of this region. */
	wpos offset;
	/* Next region in the hash table. */
	struct vxwld_region *next;
	/* Check if this region should be deleted. */
	size_t consider_deletion;
} vxwld_region;

/* Number of buckets in region hash table. */
#define VX_WLD_BUCKETS_COUNT 256u

typedef struct
{
	size_t valid_count;
	vxwld_region *buckets[VX_WLD_BUCKETS_COUNT];
} vxwld_region_table;
/* All created regions. */
extern vxwld_region_table vxwld_regions;

/* Chunks queued for meshing. */
extern vxdy_array vxwld_tomesh_queue;
/* Mesh results objects in use. */
extern vxdy_array vxwld_mesh_result_queue;

/* Main noise object. */
extern struct vxns_obj *vxwld_noise;

/* World statistics. */
typedef struct
{
	size_t rendered_tris_count;
	size_t draw_calls_count;
	size_t rendered_chunks_count;
	size_t rendered_regions_count;
	pos_type render_distance;
} vxwld_statistics;
extern vxwld_statistics vxwld_info;

VX_C_START

/* Initialize the world and other related drawn items. */
void vxwld_init(VX_NO_ARG);
/* Draw the world. */
int vxwld_draw(void *unused);

/* Setup uniform variables. */
void vxwld_setup_uniform(VX_NO_ARG);

/* World update logic. */
void vxwld_observe(VX_NO_ARG);

/* Wait for world threads to be idle and stop them from doing anything.
   If non-blocking, this will have to be repeatedly called until it returns 1. */
int vxwld_pause_threads(int blocking);
/* Resume world threads operations. */
void vxwld_resume_threads(VX_NO_ARG);
/* Remesh all chunks. */
void vxwld_queue_all_remesh(VX_NO_ARG);

VX_FORCEINLINE int vxwld_offsets_equivalent(const wpos *a, const wpos *b);
VX_FORCEINLINE size_t vxwld_regoff_bucket(pos_type x, pos_type y, pos_type z);
VX_FORCEINLINE size_t vxwld_regoff_bucket_wpos(const wpos *region_offset);

/* Whether two offsets are the same. */
VX_FORCEINLINE int vxwld_offsets_equivalent(const wpos *a, const wpos *b)
{
	return a->x == b->x && a->y == b->y && a->z == b->z;
}

#if defined (_WIN32) || !defined (__LP64__)
#define BUCKET_TYPE long long
#define BUCKET_C(x) x ## LL
#else
#define BUCKET_TYPE long
#define BUCKET_C(x) x ## L
#endif

#define VX_WLD_HASH_OFFSET (BUCKET_C(-3750763034362895579))
#define VX_WLD_HASH_PRIME (BUCKET_C(1099511628211))

/* Hash table 'bucket' of given region offset as separate components. */
VX_FORCEINLINE size_t vxwld_regoff_bucket(pos_type x, pos_type y, pos_type z)
{
	BUCKET_TYPE bucket = VX_WLD_HASH_OFFSET;
	bucket = (bucket ^ x) * VX_WLD_HASH_PRIME;
	bucket = (bucket ^ y) * VX_WLD_HASH_PRIME;
	bucket = (bucket ^ z) * VX_WLD_HASH_PRIME;
	return VX_CAST(size_t, bucket) % VX_WLD_BUCKETS_COUNT;
}
/* Hash table 'bucket' of given region offset. */
VX_FORCEINLINE size_t vxwld_regoff_bucket_wpos(const wpos *o)
{
	BUCKET_TYPE bucket = VX_WLD_HASH_OFFSET;
	bucket = (bucket ^ o->x) * VX_WLD_HASH_PRIME;
	bucket = (bucket ^ o->y) * VX_WLD_HASH_PRIME;
	bucket = (bucket ^ o->z) * VX_WLD_HASH_PRIME;
	return VX_CAST(size_t, bucket) % VX_WLD_BUCKETS_COUNT;
}

/* Find a region by the offset. */
vxwld_region *vxwld_get_region(const wpos *region_offset);
/* Find a region by the offset, returning an empty one if not found. */
vxwld_region *vxwld_get_region_empty(const wpos *region_offset);
/* Create a region at the specified region offset, or return an existing one. */
vxwld_region *vxwld_create_region(const wpos *region_offset);

/* Returns whether the given region can be seen. */
int vxwld_region_visible(const vxwld_region *region);
/* Returns whether the given chunk can be seen. */
int vxwld_chunk_visible(const vxwld_chunk *chunk);

/* Change the render distance. */
void vxwld_change_rdist(pos_type render_distance);

/* Deallocate resources used by a chunk. */
void vxwld_destroy_chunk(vxwld_chunk *chunk, int buffers_only);
/* Determine whether a region has no valid chunks. */
int vxwld_destroy_region_check(vxwld_region *region);
/* Destroy a region. */
void vxwld_destroy_region(vxwld_region *region, int destroy_chunks);
/* Destroy all regions. */
void vxwld_destroy_all_regions(VX_NO_ARG);
/* Destroy all elements relating to the world. */
void vxwld_destroy(VX_NO_ARG);

#if VX_WLD_DEBUG - 0
# include <stdio.h>
#else
# define printf(...)
#endif


VX_C_END

#endif
