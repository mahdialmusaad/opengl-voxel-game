#include "world/mesh.h"
#include "world/locate.h"
#include "world/map.h"

#include "directives/dcast.h"
#include "directives/dfree.h"
#include "directives/dfast.h"
#include "directives/dmath.h"

#include "values/elements.h"
#include "values/state.h"

#if VX_WLD_DEBUG - 0
#include "graphics/glfw.h"
#endif

#include "utils/thread.h"

#include "io/logs.h"

#include <stdlib.h>
#include <string.h>

#define VX_FILE_ID "mesh.c"

/* Also storing blocks that are bordering the chunk, so +2 per axis. */
#define VX_MESH_CHUNK_XBLKS (VX_WLD_CHUNK_XBLKS + 2)
#define VX_MESH_CHUNK_YBLKS (VX_WLD_CHUNK_YBLKS + 2)
#define VX_MESH_CHUNK_ZBLKS (VX_WLD_CHUNK_ZBLKS + 2)
#define VX_MESH_CHUNK_COUNT (VX_MESH_CHUNK_XBLKS * VX_MESH_CHUNK_YBLKS * VX_MESH_CHUNK_ZBLKS)

typedef struct {
	vxdy_array local_mesh_result_queue;// = VX_DYARRAY_INIT(sizeof(vxwld_mesh_result), 1);
	vxwld_mesh_result local_res;
	vxblk *surrounding_blocks;
	size_t start, end;
} vxwld_mesh_thread;

static VX_THREAD_FUNCTION(vxwld_mesh_inner)
{
	vxwld_mesh_thread *mthread_data = VX_CAST(vxwld_mesh_thread *, thread_arg);

	mthread_data->surrounding_blocks = VX_CAST(vxblk *, calloc(VX_CAST(size_t, VX_MESH_CHUNK_COUNT), sizeof *mthread_data->surrounding_blocks));
	if (!mthread_data->surrounding_blocks) VX_ABORT_ALLOCATION();

	mthread_data->local_mesh_result_queue.element_size = sizeof(vxwld_mesh_result);
	mthread_data->local_mesh_result_queue.reserve_double = 1u;

	for (size_t i = mthread_data->start; i < mthread_data->end; ++i) {
		vxwld_chunk *chunk = VX_CAST(vxwld_chunk **, vxwld_tomesh_queue.data)[i];
		chunk->is_queued = 0u;

		memset(&mthread_data->local_res, 0, sizeof mthread_data->local_res);
		memset(mthread_data->surrounding_blocks, 0,VX_CAST(size_t, VX_MESH_CHUNK_COUNT) * sizeof *mthread_data->surrounding_blocks);
		mthread_data->local_res.chunk = chunk;

		if (!vxwld_mesh(&mthread_data->local_res, mthread_data->surrounding_blocks)) continue;
		vxdy_array_add(&mthread_data->local_mesh_result_queue, &mthread_data->local_res);
	}

	VX_FREE(mthread_data->surrounding_blocks);
	return VX_THREAD_RETURN_VALUE;
}

size_t vxwld_mesh_changes(VX_NO_ARG)
{
	if (!vxwld_tomesh_queue.size) return 0;
	const size_t target_work = VX_INT_MIN(vxwld_tomesh_queue.size, VX_WLD_MAX_MESH);
	if (!target_work) return 0u;

	const size_t workers_count = VX_CAST(size_t, vxstate_vals.available_threads);
	const size_t created_workers_count = workers_count - 1;
	vxwld_mesh_thread *mesh_threads = VX_CAST(vxwld_mesh_thread *, calloc(workers_count, sizeof *mesh_threads));
	VX_THREAD_TYPE *mesh_threads_ids = VX_CAST(VX_THREAD_TYPE *, calloc(created_workers_count, sizeof *mesh_threads_ids));
	if (!mesh_threads || !mesh_threads_ids) return 0u;

#if VX_WLD_DEBUG - 0
	size_t total_actual_done = 0u;
	const double meshing_start = glfwGetTime();
	size_t threads_used = 1u;
#endif

	const size_t even_split = target_work / workers_count;
	const size_t left_over = target_work - (even_split * workers_count);

	for (size_t i = 0; i < workers_count; ++i) {
		mesh_threads[i].start = even_split * i + (i >= left_over ? left_over : i);
		const size_t next = i + 1; 
		mesh_threads[i].end = even_split * next + (next >= left_over ? left_over : next);

		const int force_outer_thread = i == created_workers_count;
		if (!force_outer_thread) mesh_threads_ids[i] = vxthr_create_thread(vxwld_mesh_inner, mesh_threads + i);
		/* Do the last work and the work of any mesh thread that failed to be created. */
		if (force_outer_thread || !mesh_threads_ids[i]) vxwld_mesh_inner(mesh_threads + created_workers_count);
	#if VX_WLD_DEBUG - 0
		else ++threads_used;
	#endif
	}
	for (size_t i = 0; i < created_workers_count; ++i) if (mesh_threads_ids[i]) vxthr_join_thread(mesh_threads_ids[i]);

	/* Combine all local queues. */
	for (size_t i = 0; i < workers_count; ++i) {
	#if VX_WLD_DEBUG - 0
		total_actual_done += mesh_threads[i].local_mesh_result_queue.size;
	#endif
		vxdy_array_addmult(&vxwld_mesh_result_queue, mesh_threads[i].local_mesh_result_queue.data, mesh_threads[i].local_mesh_result_queue.size);
		vxdy_array_free(&mesh_threads[i].local_mesh_result_queue);
	}

	if (target_work == vxwld_tomesh_queue.size) vxdy_array_free(&vxwld_tomesh_queue);
	else vxdy_array_remove(&vxwld_tomesh_queue, 0u, target_work);

#if VX_WLD_DEBUG - 0
	const double mesh_elapsed = (glfwGetTime() - meshing_start) * 1000.0;
	const double work_done = VX_CAST(double, total_actual_done + (total_actual_done == 0));
	wdbg_printf("[ MESHING ] Meshed %zu of which %zu were valid (%zu threads, %.2fms/chunk/thread, %.2fms total); %zu left\n",
		target_work, total_actual_done, threads_used, mesh_elapsed / work_done, mesh_elapsed, vxwld_tomesh_queue.size
	);
#endif

	return target_work;
}

/* Indexed positions of vector directions in a 'mesh chunk', both with and without the
   'origin offset' (needed as index 0 points to relative block 0,0,0 not 0,0,0). */
static const intmax_t vxwld_dir_indices[6] = {
	#define VX_MESH_ORIGIN_INDEX VX_INDEX_FROM_XYZ(1, 1, 1, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS)
	VX_INDEX_FROM_XYZ( 1,  0,  0, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS) + VX_MESH_ORIGIN_INDEX,
	VX_INDEX_FROM_XYZ( 0,  1,  0, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS) + VX_MESH_ORIGIN_INDEX,
	VX_INDEX_FROM_XYZ( 0,  0,  1, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS) + VX_MESH_ORIGIN_INDEX,
	VX_INDEX_FROM_XYZ(-1,  0,  0, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS) + VX_MESH_ORIGIN_INDEX,
	VX_INDEX_FROM_XYZ( 0, -1,  0, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS) + VX_MESH_ORIGIN_INDEX,
	VX_INDEX_FROM_XYZ( 0,  0, -1, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS) + VX_MESH_ORIGIN_INDEX
};

/* Determines darkness from nearby blocks. */
static inline void vxwld_determine_ambientocc(const vxblk *surrounding_blocks, intmax_t current_ind, int face, uint32_t *ao_corners_darkness)
{
	/* Precalculated ambient occlusion adjacent offsets to improve compile times and support MSVC. */
	static const struct vxwld_ao_data { int leftmost, rightmost, diagonal; } vxwld_ao_adjacent_positions[6][4] = {
        	{ { 2348, 2381, 2382 }, { 2381, 2346, 2380 }, { 2346, 2313, 2312 }, { 2313, 2348, 2314 } },
		{ { 1224, 2381, 2380 }, { 2381, 1226, 2382 }, { 1226, 69,   70   }, { 69,   1224, 68   } },
		{ { 36,   1226, 70   }, { 1226, 2348, 2382 }, { 2348, 1158, 2314 }, { 1158, 36,   2    } },
		{ { 34,   69,   68   }, { 69,   36,   70   }, { 36,   1,    2    }, { 1,    34,   0    } },
		{ { 1156, 1,    0    }, { 1,    1158, 2    }, { 1158, 2313, 2314 }, { 2313, 1156, 2312 } },
		{ { 2346, 1224, 2380 }, { 1224, 34,   68   }, { 34,   1156, 0    }, { 1156, 2346, 2312 } }
	};

	const struct vxwld_ao_data *ao_offset = vxwld_ao_adjacent_positions[face];

	VX_UNROLL(4)
	for (int corner_ind = 0; corner_ind < 4; ++corner_ind, ++ao_offset) {
		const vxelm_attribs *leftmost = vxelm_elements + surrounding_blocks[current_ind + ao_offset->leftmost];
		const vxelm_attribs *rightmost = vxelm_elements + surrounding_blocks[current_ind + ao_offset->rightmost];
		const vxelm_attribs *diagonal = vxelm_elements + surrounding_blocks[current_ind + ao_offset->diagonal];
	
		/* Make the corner darker depending on how many adjacent non-air blocks there are. */
		uint32_t accumulated_darkness = VX_CAST(uint32_t, leftmost != vxelm_elements) + (rightmost != vxelm_elements);
		if (diagonal && accumulated_darkness < 2) accumulated_darkness += diagonal != vxelm_elements;
		ao_corners_darkness[corner_ind] = accumulated_darkness * 5u;
	}
}

int vxwld_mesh(vxwld_mesh_result *result, vxblk *surrounding_blocks)
{
	if (!result || !result->chunk->blocks.data) return 1;

	/* How much more general data to allocate if there isn't enough. */
	#define VX_MESH_EXTRA_ALLOC (512)
	/* How much general data to start with. */
	#define VX_MESH_START_ALLOC (8192)

	struct {
		size_t opaque_mesh_capacity, translucent_mesh_capacity;
		size_t opaque_ebo_capacity, translucent_ebo_capacity;
		uint32_t opaque_mesh_count, translucent_mesh_count;
		vxwld_ebo opaque_largest_index, translucent_largest_index;
	} counters;
	memset(&counters, 0, sizeof counters);

	wpos chunk_global_position, global_pos_back = { -1, -1, -1 };
	vxwld_chunk_globpos(result->chunk, &chunk_global_position);
	wpos_add(&global_pos_back, &global_pos_back, &chunk_global_position);

	wpos chunk_region_pos;
	vxwld_chunk_regpos(result->chunk, &chunk_region_pos);
	
	vxblk *surrounding_ptr = surrounding_blocks;
	VX_UNROLL(4)
	for (intmax_t i = 0; i < VX_MESH_CHUNK_COUNT; ++i) {
		const pos_type gpos_x = global_pos_back.x + VX_XPOS_FROM_INDEX(i, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS);
		const pos_type gpos_y = global_pos_back.y + VX_YPOS_FROM_INDEX(i, VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS);
		const pos_type gpos_z = global_pos_back.z + VX_ZPOS_FROM_INDEX(i, VX_MESH_CHUNK_ZBLKS);

		const wpos region_offset = {
			VX_GPOS_TO_GOFF(gpos_x, VX_WLD_REGION_XBLKS),
			VX_GPOS_TO_GOFF(gpos_y, VX_WLD_REGION_YBLKS),
			VX_GPOS_TO_GOFF(gpos_z, VX_WLD_REGION_ZBLKS)
		};
		vxwld_region *kv_region = vxwld_regions.buckets[vxwld_regoff_bucket_wpos(&region_offset)];
		for (; kv_region; kv_region = kv_region->next) if (VX_LIKELY(vxwld_offsets_equivalent(&region_offset, &kv_region->offset))) goto found_region;
		*(surrounding_ptr++) = 0;
		continue;
	found_region:
		{
			vxwld_chunk *chunk = kv_region->chunks + VX_CHUNK_REGION_INDEX(
				VX_GPOS_TO_LOFF(gpos_x, VX_WLD_CHUNK_XBLKS, VX_WLD_REGION_XDIM),
				VX_GPOS_TO_LOFF(gpos_y, VX_WLD_CHUNK_YBLKS, VX_WLD_REGION_YDIM),
				VX_GPOS_TO_LOFF(gpos_z, VX_WLD_CHUNK_ZBLKS, VX_WLD_REGION_ZDIM)
			);
			*(surrounding_ptr++) = !chunk->blocks.data ? 0 : chunk->blocks.data[VX_POS_AS_INDEX_SEP(gpos_x, gpos_y, gpos_z)];	
		}
	}

	vxwld_render *opaque_mesh = VX_CAST(vxwld_render *, malloc(sizeof *opaque_mesh * (counters.opaque_mesh_capacity = VX_MESH_START_ALLOC * 4u)));
	vxwld_render *translucent_mesh = VX_CAST(vxwld_render *, malloc(sizeof *translucent_mesh * (counters.translucent_mesh_capacity = VX_MESH_START_ALLOC * 4u))); 
	if (!opaque_mesh || !translucent_mesh) {
		VX_FREE(opaque_mesh);
		return 0;
	}

	vxwld_ebo *opaque_ebo = VX_CAST(vxwld_ebo *, malloc(sizeof *opaque_ebo * (counters.opaque_ebo_capacity = VX_MESH_START_ALLOC * 6u)));
	vxwld_ebo *translucent_ebo = VX_CAST(vxwld_ebo *, malloc(sizeof *translucent_ebo * (counters.translucent_ebo_capacity = VX_MESH_START_ALLOC * 6u)));
	if (!opaque_ebo || !translucent_ebo) {
		VX_FREE(opaque_mesh);
		VX_FREE(translucent_mesh);
		VX_FREE(opaque_ebo);
		return 0;
	}

	/* Loop through all blocks to determine visibility. */
	for (intmax_t i = 0u; i < VX_WLD_CHUNK_BLOCKS; ++i) {
		const vxelm_attribs *local = vxelm_elements + result->chunk->blocks.data[i];
		if (local == vxelm_elements) continue;

		/* Switch from the normal chunk 1D index to a 'mesh chunk' index (includes outer blocks). The 'direction'
		   indices already have an added origin offset to them (go from 0,0,0 block on index 0 to 0,0,0 block). */
		const wpos normal_local_pos = {
			VX_XPOS_FROM_INDEX(i, VX_WLD_CHUNK_YBLKS, VX_WLD_CHUNK_ZBLKS),
			VX_YPOS_FROM_INDEX(i, VX_WLD_CHUNK_YBLKS, VX_WLD_CHUNK_ZBLKS),
			VX_ZPOS_FROM_INDEX(i, VX_WLD_CHUNK_ZBLKS)
		};
		const wpos region_pos = {
			chunk_region_pos.x + normal_local_pos.x,
			chunk_region_pos.y + normal_local_pos.y,
			chunk_region_pos.z + normal_local_pos.z
		};
		const intmax_t mesh_local_ind = VX_INDEX_FROM_XYZ(
			normal_local_pos.x, normal_local_pos.y, normal_local_pos.z,
			VX_MESH_CHUNK_YBLKS, VX_MESH_CHUNK_ZBLKS
		);

		/* Add vertices and indices of each face if determined to be visible against the corresponding adjacent block. */
		for (int face = 0; face < 6; ++face) {
			/* Add the corresponding 3D index of the specific face vector and origin offset. */
			const vxelm_attribs *adjacent = vxelm_elements + surrounding_blocks[mesh_local_ind + vxwld_dir_indices[face]];
			/* An opaque block is visible against a translucent block; a translucent block is visible against a different translucent block. */
			if (VX_LIKELY(!(!local->transparency && adjacent->transparency) && !(local->transparency && adjacent->transparency && local != adjacent))) continue;

			vxwld_render **target_mesh = local->transparency ? &translucent_mesh : &opaque_mesh;
			vxwld_ebo **target_ebo = local->transparency ? &translucent_ebo : &opaque_ebo;

			vxwld_ebo *target_largest_index = local->transparency ? &counters.translucent_largest_index : &counters.opaque_largest_index;
			size_t *target_mesh_capacity = local->transparency ? &counters.translucent_mesh_capacity : &counters.opaque_mesh_capacity;
			size_t *target_ebo_capacity = local->transparency ? &counters.translucent_ebo_capacity : &counters.opaque_ebo_capacity;
			uint32_t *target_ebo_count = local->transparency ? &result->translucent_indices_count : &result->opaque_indices_count;
			uint32_t *target_mesh_count = local->transparency ? &counters.translucent_mesh_count : &counters.opaque_mesh_count;

			/* Allocate vertices and indices if there isn't enough. */
			const size_t verts_count = VX_CAST(size_t, local->mesh->verts_cnts[face]);
			const size_t ebos_count = VX_CAST(size_t, local->mesh->ebos_cnts[face]);

			if (VX_UNLIKELY(*target_mesh_count + verts_count > *target_mesh_capacity)) {
				void *const rlc_mesh = realloc(*target_mesh, sizeof(vxwld_render) * (*target_mesh_capacity += verts_count * VX_MESH_EXTRA_ALLOC));
				if (!rlc_mesh) goto error_out;
				else if (target_mesh != rlc_mesh) *target_mesh = VX_CAST(vxwld_render *, rlc_mesh);
			}
			if (VX_UNLIKELY(*target_ebo_count + ebos_count > *target_ebo_capacity)) {
				void *const rlc_ebo = realloc(*target_ebo, sizeof(vxwld_ebo) * (*target_ebo_capacity += ebos_count * VX_MESH_EXTRA_ALLOC));
				if (!rlc_ebo) goto error_out;
				else if (*target_ebo != rlc_ebo) *target_ebo = VX_CAST(vxwld_ebo *, rlc_ebo);
			}

			uint32_t ao_corners_darkness[4];
			vxwld_determine_ambientocc(surrounding_blocks, mesh_local_ind, face, ao_corners_darkness);

			const vxelm_mesh_vertex *vert = local->mesh->verts[face];
			const uint32_t tex_vpos_start = (local->textures[face] * 16u);

			/* Add each vertex of the given block. */
			for (size_t v = 0u; v < verts_count; ++v, ++vert) {
				vxwld_render *cur = *target_mesh + (*target_mesh_count)++;

				struct { uint64_t x : 20, y : 20, z : 24; } packed_pos;
				packed_pos.x = VX_CAST(uint64_t, 16.0f * (VX_CAST(float, region_pos.x) + vert->x)) & 0xFFFFFu;
				packed_pos.y = VX_CAST(uint64_t, 16.0f * (VX_CAST(float, region_pos.y) + vert->y)) & 0xFFFFFu;
				packed_pos.z = VX_CAST(uint64_t, 16.0f * (VX_CAST(float, region_pos.z) + vert->z)) & 0xFFFFFu;

				cur->packed_pos_xy = VX_REINT_CAST(uint32_t *, &packed_pos)[0];
				cur->packed_pos_yz = VX_REINT_CAST(uint32_t *, &packed_pos)[1] & 0xFFFFFFFu;

				cur->light_world = (15u - ao_corners_darkness[v]) & 0xFu;

				cur->light_r = 0u;
				cur->light_g = 0u;
				cur->light_b = 0u;

				cur->texture_v_id = (tex_vpos_start + VX_CAST(uint32_t, vert->v * 16.0f)) & 0xFFFFu;
				cur->texture_u = VX_CAST(uint32_t, vert->u * 15.0f) & 0xFu;
			}

			/* Add indices and advance. */
			const unsigned int *local_ebos = local->mesh->ebos[face];
			for (size_t e = 0u; e < ebos_count; ++e) (*target_ebo)[(*target_ebo_count)++] = *target_largest_index + *local_ebos++;
			*target_largest_index += VX_CAST(vxwld_ebo, verts_count);
		}
	}
error_out:
	/* Ensure translucent indices start after opaque data. */
	for (size_t i = 0u; i < result->translucent_indices_count; ++i) translucent_ebo[i] += counters.opaque_largest_index;
	uint32_t total_indices = 0u;

	/* Combine opaque and translucent data into one array. */
	if ((result->mesh_count = counters.opaque_mesh_count + counters.translucent_mesh_count)) {
		result->overall_mesh = VX_CAST(vxwld_render *, realloc(opaque_mesh, sizeof *result->overall_mesh * result->mesh_count));
		if (!result->overall_mesh) goto error_free;
		opaque_mesh = VX_NULL;
		memcpy(result->overall_mesh + counters.opaque_mesh_count, translucent_mesh, sizeof *translucent_mesh * counters.translucent_mesh_count);
	}
	if ((total_indices = result->opaque_indices_count + result->translucent_indices_count)) {
		result->overall_ebo = VX_CAST(vxwld_ebo *, realloc(opaque_ebo, sizeof *result->overall_ebo * total_indices));
		if (!result->overall_ebo) goto error_free;
		opaque_ebo = VX_NULL;
		memcpy(result->overall_ebo + result->opaque_indices_count, translucent_ebo, sizeof *translucent_ebo * result->translucent_indices_count);
	}

	VX_FREE(translucent_mesh);
	VX_FREE(translucent_ebo);
	return 1;
error_free:
	VX_FREE(opaque_ebo);
	VX_FREE(opaque_mesh);
	VX_FREE(result->overall_mesh);
	VX_FREE(result->overall_ebo);
	VX_FREE(translucent_mesh);
	VX_FREE(translucent_ebo);

	memset(result, 0, sizeof *result);
	return 0;
}
