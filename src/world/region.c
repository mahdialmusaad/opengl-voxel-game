#include "world/map.h"
#include "world/modify.h"
#include "world/locate.h"

#include "directives/dcast.h"
#include "directives/dfree.h"

#include "graphics/glfuncs.h"

#include "player/frustum.h"
#include "player/camera.h"

#include <string.h>
#include <stdlib.h>

vxwld_region_table vxwld_regions;

vxwld_region *vxwld_get_region(const wpos *region_offset)
{
	vxwld_region *kv_region = vxwld_regions.buckets[vxwld_regoff_bucket_wpos(region_offset)];
	for (; kv_region; kv_region = kv_region->next) {
		if (vxwld_offsets_equivalent(region_offset, &kv_region->offset)) return kv_region;
	}
	return VX_NULL;
}

vxwld_region *vxwld_create_region(const wpos *region_offset)
{
	vxwld_region *region = vxwld_get_region(region_offset);
	if (region) return region;

	if (!(region = VX_CAST(vxwld_region *, calloc(1u, sizeof *region)))) return VX_NULL;

	vxwld_region **kv_region = vxwld_regions.buckets + vxwld_regoff_bucket_wpos(region_offset);

	while (*kv_region) kv_region = &(*kv_region)->next;
	*kv_region = region;

	++vxwld_regions.valid_count;
	region->offset = *region_offset;

	for (uint32_t c = 0u; c < VX_WLD_REGION_ALLDIM; ++c) {
		vxwld_chunk *chunk = region->chunks + c;
		chunk->region_index = c & 0x1FFFFFFFu;
	}

	return region;
}


void vxwld_destroy_chunk(vxwld_chunk *chunk, int buffers_only)
{
	if (chunk->vao) {
		gl.DeleteVertexArrays(1, &chunk->vao);
		chunk->vao = 0u;
		gl.DeleteBuffers(1, &chunk->vbo);
		chunk->vbo = 0u;
		gl.DeleteBuffers(1, &chunk->ebo);
		chunk->ebo = 0u;
	}

	if (buffers_only) return;

	const uint32_t saved_index = chunk->region_index;
	VX_FREE(chunk->blocks.data);
	memset(chunk, 0, sizeof *chunk);
	chunk->region_index = saved_index & 0x1FFFFFFFu;
}

int vxwld_destroy_region_check(vxwld_region *region)
{
	for (size_t c = 0; c < VX_WLD_REGION_ALLDIM; ++c) if (region->chunks[c].is_decorated) return 0;
	return 1;
}

static void vxwld_destroy_region_chunks(vxwld_region *region)
{
	for (size_t c = 0; c < VX_WLD_REGION_ALLDIM; ++c) vxwld_destroy_chunk(region->chunks + c, 0);
	VX_FREE(region);
}

void vxwld_destroy_region(vxwld_region *region, int destroy_chunks)
{
	const wpos target_offset = region->offset;
	vxwld_region *kv_region = vxwld_regions.buckets[vxwld_regoff_bucket_wpos(&target_offset)];
	vxwld_region *prev_kv_region = VX_NULL;

	if (destroy_chunks) vxwld_destroy_region_chunks(region);

	while (kv_region) {
		if (vxwld_offsets_equivalent(&kv_region->offset, &target_offset)) {
			if (prev_kv_region) prev_kv_region->next = kv_region->next;
			return;
		}
		prev_kv_region = kv_region;
		kv_region = kv_region->next;
	}
}

void vxwld_destroy_all_regions(VX_NO_ARG)
{
	for (size_t bucket_ind = 0u; bucket_ind < VX_WLD_BUCKETS_COUNT; ++bucket_ind) {
		vxwld_region *kv_region = vxwld_regions.buckets[bucket_ind];
		while (kv_region) {
			vxwld_region *next = kv_region->next; 
			vxwld_destroy_region_chunks(kv_region);
			kv_region = next;
		}
	}
}


int vxwld_region_visible(const vxwld_region *region)
{
	wpos region_ncorner;
	vxwld_regoff_globpos(&region->offset, &region_ncorner);
	const dvec3 chunk_ncorner_db = {
		VX_CAST(double, region_ncorner.x),
		VX_CAST(double, region_ncorner.y),
		VX_CAST(double, region_ncorner.z)
	};
	return vxfrs_cuboid_visible(
		&vxplr_cam.frustum, &chunk_ncorner_db,
		VX_WLD_REGION_XBLKS, VX_WLD_REGION_YBLKS, VX_WLD_REGION_ZBLKS
	);
}
int vxwld_chunk_visible(const vxwld_chunk *chunk)
{
	wpos chunk_ncorner;
	vxwld_chunk_globpos(chunk, &chunk_ncorner);
	const dvec3 chunk_ncorner_db = {
		VX_CAST(double, chunk_ncorner.x),
		VX_CAST(double, chunk_ncorner.y),
		VX_CAST(double, chunk_ncorner.z)
	};
	return vxfrs_cuboid_visible(
		&vxplr_cam.frustum, &chunk_ncorner_db,
		VX_WLD_CHUNK_XBLKS, VX_WLD_CHUNK_YBLKS, VX_WLD_CHUNK_ZBLKS
	);
}


void vxwld_change_rdist(pos_type render_distance)
{
	vxwld_info.render_distance = render_distance;
	if (vxwld_info.render_distance < 4) vxwld_info.render_distance = 4;
}

void vxwld_queue_all_remesh(VX_NO_ARG)
{
	for (size_t i = 0u; i < VX_WLD_BUCKETS_COUNT; ++i) {
		vxwld_region *kv_region = vxwld_regions.buckets[i];
		for (; kv_region; kv_region = kv_region->next) {
			for (size_t c = 0; c < VX_WLD_REGION_ALLDIM; ++c) {
				vxwld_chunk *chunk = kv_region->chunks + c;
				if (!chunk->opaque_indices_count && !chunk->translucent_indices_count) continue;
				vxwld_queue_change(chunk);
			}
		}
	}
}
