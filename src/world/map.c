#include "world/map.h"
#include "world/generate.h"
#include "world/locate.h"
#include "world/mesh.h"

#include "directives/dcast.h"
#include "directives/dword.h"
#include "directives/dfree.h"
#include "directives/dmath.h"

#include "shaders/programs.h"

#include "graphics/glfuncs.h"
#include "graphics/glenum.h"
#include "graphics/glctx.h"

#include "player/movement.h"

#include "values/state.h"

#include "utils/thread.h"
#include "utils/noise.h"

#include "vector/vec3.h"

#include <stdlib.h>
#include <string.h>

static VX_THREAD_FUNCTION(vxwld_gthread_loop);
static void vxwld_borders_init(VX_NO_ARG);
static void vxwld_axis_init(VX_NO_ARG);

typedef struct {
	vxwld_region *region;
	vxdy_array chunks;
	double region_db_pos[3];
	float draw_sub_pos[3];
	int not_visible;
} vxwld_render_region;
static vxdy_array vxwld_render_regions = VX_DYARRAY_INIT(sizeof(vxwld_render_region), 1);

vxdy_array vxwld_mesh_result_queue = VX_DYARRAY_INIT(sizeof(vxwld_mesh_result), 1);
vxdy_array vxwld_tomesh_queue = VX_DYARRAY_INIT(sizeof(vxwld_chunk *), 1);

static vxthr_exchangeable vxwld_result_exchange;

vxwld_statistics vxwld_info;

static struct vxns_obj vxwld_noise_value;
struct vxns_obj *vxwld_noise;

static volatile int world_active;
typedef struct { volatile int paused, active; } vxwld_thread_state;
static vxwld_thread_state vxwld_gthread = { 0, -1 };

static int vxwld_region_pos_location;

void vxwld_init(VX_NO_ARG)
{
	world_active = 1u;

	vxns_init(vxwld_noise = &vxwld_noise_value);
	
	vxctx_init(&vxsd_shaders.blocks,
		VX_NULL, 1u, VX_NULL, VX_CTX_NVBO, VX_CTX_NEBO, VX_NULL,
		vxen_ctxorder_world, vxen_ctxmode_tris, VX_NULL, 0, vxwld_draw
	);

	vxwld_change_rdist(4);
	vxwld_borders_init();
	vxwld_axis_init();

	vxthr_exchange_init(&vxwld_result_exchange);
	vxthr_detach_thread(vxthr_create_thread(vxwld_gthread_loop, &vxwld_gthread));

	vxwld_setup_uniform();
}

int vxwld_draw(void *unused)
{
	(void)(unused);

	gl.UseProgram(vxsd_shaders.blocks.pid);
	vxwld_info.rendered_tris_count = 0u;
	vxwld_info.draw_calls_count = 0u;

	for (size_t i = 0u; i < vxwld_render_regions.size; ++i) {
		vxwld_render_region *render = VX_CAST(vxwld_render_region *, vxwld_render_regions.data) + i;
		render->draw_sub_pos[0] = VX_CAST(float, render->region_db_pos[0] - vxplr_inst.pos.x);
		render->draw_sub_pos[1] = VX_CAST(float, render->region_db_pos[1] - vxplr_inst.pos.y);
		render->draw_sub_pos[2] = VX_CAST(float, render->region_db_pos[2] - vxplr_inst.pos.z);
		vxsd_set_vec3(VX_NULL, vxwld_region_pos_location, render->draw_sub_pos);

		for (size_t c = 0u; c < render->chunks.size; ++c) {
			vxwld_chunk *chunk = VX_CAST(vxwld_chunk **, render->chunks.data)[c];
			if (!chunk->opaque_indices_count) continue;

			vxwld_info.rendered_tris_count += chunk->opaque_indices_count;
			++vxwld_info.draw_calls_count;

			gl.BindVertexArray(chunk->vao);
			gl.DrawElements(
				GL_TRIANGLES,
				VX_CAST(GLsizei, chunk->opaque_indices_count),
				GL_UNSIGNED_INT,
				VX_NULL
			);
		}
	}

	for (size_t i = 0u; i < vxwld_render_regions.size; ++i) {
		vxwld_render_region *render = VX_CAST(vxwld_render_region *, vxwld_render_regions.data) + i;
		vxsd_set_vec3(VX_NULL, vxwld_region_pos_location, render->draw_sub_pos);

		for (size_t c = 0u; c < render->chunks.size; ++c) {
			vxwld_chunk *chunk = VX_CAST(vxwld_chunk **, render->chunks.data)[c];
			if (!chunk->translucent_indices_count) continue;

			vxwld_info.rendered_tris_count += chunk->translucent_indices_count;
			++vxwld_info.draw_calls_count;
			gl.BindVertexArray(chunk->vao);
			gl.DrawElements(
				GL_TRIANGLES,
				VX_CAST(GLsizei, chunk->translucent_indices_count),
				GL_UNSIGNED_INT,
				VX_CAST(const void *, chunk->opaque_indices_count * sizeof(uint32_t))
			);
		}
	}

	vxwld_info.rendered_tris_count /= 3;
	return 2;
}


void vxwld_setup_uniform(VX_NO_ARG)
{
	vxwld_region_pos_location = vxsd_get_location(&vxsd_shaders.blocks, "region_pos");
	vxsd_set_floats(&vxsd_shaders.blocks, vxsd_get_location(&vxsd_shaders.blocks, "tex_y_pixel"), &vxelm_block_texture_y_pixel, 1);
}


/* Buffer data from mesh results as determined by the mesh thread. */
static size_t vxwld_apply_mesh_changes(VX_NO_ARG)
{
	const size_t consume_count = VX_INT_MIN(vxwld_mesh_result_queue.size, VX_WLD_MAX_CONSUME);
	if (!consume_count) return 0u;

	if (!vxthr_exchange_lock(&vxwld_result_exchange, 0)) return 0u;
#if VX_WLD_DEBUG - 0
	size_t empty_count = 0u;
#endif

	for (size_t i = 0u; i < consume_count; ++i) {
		vxwld_mesh_result *result = VX_CAST(vxwld_mesh_result *, vxwld_mesh_result_queue.data) + i;
		vxwld_chunk *chunk = result->chunk;

		const uint32_t original_indices_count = chunk->opaque_indices_count + chunk->translucent_indices_count;
		const uint32_t total_indices_count = result->opaque_indices_count + result->translucent_indices_count;
		chunk->translucent_indices_count = result->translucent_indices_count;
		chunk->opaque_indices_count = result->opaque_indices_count;

		const int needs_buffers = !chunk->vao;

		if (!total_indices_count) {
			vxwld_destroy_chunk(chunk, 1);
		#if VX_WLD_DEBUG - 0
			++empty_count;
		#endif
			goto immediate_clear;
		}

		/* Initialize buffers if they haven't been already. */
		if (needs_buffers) {
			gl.GenVertexArrays(1, &chunk->vao);
			gl.GenBuffers(1, &chunk->vbo);
			gl.GenBuffers(1, &chunk->ebo);
		}

		/* Update correct buffers. */
		gl.BindVertexArray(chunk->vao);
		gl.BindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
		gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->ebo);

		/* Also set attributes from shader if initializing (must be after binding VBO). */
		if (needs_buffers) vxctx_apply_attribs(&vxsd_shaders.blocks, 0, 0);

		if (total_indices_count > original_indices_count) {
			gl.BufferData(GL_ARRAY_BUFFER, VX_CAST(GLsizeiptr, sizeof *result->overall_mesh * result->mesh_count), result->overall_mesh, GL_STATIC_DRAW);
			gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, VX_CAST(GLsizeiptr, sizeof *result->overall_ebo * total_indices_count), result->overall_ebo, GL_STATIC_DRAW);
		} else {
			gl.BufferSubData(GL_ARRAY_BUFFER, 0, VX_CAST(GLsizeiptr, sizeof *result->overall_mesh * result->mesh_count), result->overall_mesh);
			gl.BufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, VX_CAST(GLsizeiptr, sizeof *result->overall_ebo * total_indices_count), result->overall_ebo);
		}
	
	immediate_clear:
		VX_FREE(result->overall_mesh);
		VX_FREE(result->overall_ebo);
	}

	printf("[ MAIN ] Applied %zu mesh changes of which %zu were empty\n", consume_count, empty_count);

	/* Overwrite applied changes. */
	if (consume_count != vxwld_mesh_result_queue.size) {
		vxwld_mesh_result *results_array = VX_CAST(vxwld_mesh_result *, vxwld_mesh_result_queue.data);
		memmove(
			results_array,
			results_array + consume_count,
			sizeof *results_array * (vxwld_mesh_result_queue.size - consume_count)
		);
		vxwld_mesh_result_queue.size -= consume_count;
	} else vxdy_array_free(&vxwld_mesh_result_queue);

	vxthr_exchange_unlock(&vxwld_result_exchange);

	return consume_count;
}

static void vxwld_clear_render_regions(VX_NO_ARG)
{
	for (size_t i = 0u; i < vxwld_render_regions.size; ++i) {
		vxwld_render_region *render = VX_CAST(vxwld_render_region *, vxwld_render_regions.data) + i;
		vxdy_array_free(&render->chunks);
	}
	vxdy_array_free(&vxwld_render_regions);
}

void vxwld_observe(VX_NO_ARG)
{
	size_t work_done = vxwld_apply_mesh_changes();
	if (!work_done) return;

	vxwld_clear_render_regions();
	vxwld_info.rendered_chunks_count = 0u;
	vxwld_info.rendered_regions_count = 0u;

	vxwld_render_region *render = VX_NULL;

	for (size_t i = 0u; i < VX_WLD_BUCKETS_COUNT; ++i) {
		vxwld_region *kv_region = vxwld_regions.buckets[i];
		for (; kv_region; kv_region = kv_region->next) {
			if (!render && !(render = VX_CAST(vxwld_render_region *, vxdy_array_add(&vxwld_render_regions, VX_NULL)))) continue;
			++vxwld_info.rendered_regions_count;

			memset(render, 0, sizeof *render);
			render->chunks.reserve_double = 1u;
			render->chunks.element_size = sizeof(vxwld_chunk *);
			render->region = kv_region;

			wpos region_ipos;
			vxwld_regoff_globpos(&render->region->offset, &region_ipos);
			render->region_db_pos[0] = VX_CAST(double, region_ipos.x);
			render->region_db_pos[1] = VX_CAST(double, region_ipos.y);
			render->region_db_pos[2] = VX_CAST(double, region_ipos.z);

			for (size_t c = 0u; c < VX_WLD_REGION_ALLDIM; ++c) {
				vxwld_chunk *chunk = kv_region->chunks + c;
				if (!chunk->opaque_indices_count && !chunk->translucent_indices_count) continue;
				size_t has_added = 0u;
				has_added |= vxdy_array_add(&render->chunks, &chunk) != VX_NULL;
				vxwld_info.rendered_chunks_count += has_added;
			}

			if (render->chunks.size) render = VX_NULL;
		}
	}

	if (render) vxdy_array_remove(&vxwld_render_regions, vxwld_render_regions.size - 1u, 1u);
}


static VX_THREAD_FUNCTION(vxwld_gthread_loop)
{
	vxwld_thread_state *state = VX_CAST(vxwld_thread_state *, thread_arg);
	state->active = world_active;
	size_t work_done;

	while (world_active) {
		work_done = 0u;
		do {
			work_done += vxwld_generate_around();
		} while (!vxthr_exchange_lock(&vxwld_result_exchange, !vxtg_toggles.generation_active));
		work_done += vxwld_mesh_changes();
		vxthr_exchange_unlock(&vxwld_result_exchange);

		for (state->active = 0; world_active;) {
			vxthr_wait_milli(VX_CAST(unsigned int, VX_WLD_MILLISECONDS_REST / ((work_done != 0) + 1)));
			if (!state->paused) break;
		}
		state->active = 1;
	}

	state->active = -1;
	return VX_THREAD_RETURN_VALUE;
}


int vxwld_pause_threads(int blocking)
{
	vxwld_gthread.paused = 1;
	if (!blocking) return vxwld_gthread.active != 1;
	else while (vxwld_gthread.active == 1);
	return 1;
}
void vxwld_resume_threads(VX_NO_ARG)
{
	vxwld_gthread.paused = 0;
}


/* Initialize chunk borders. */
static void vxwld_borders_init(VX_NO_ARG)
{
	const float border_shape_lines[] = {
		0.0f, 0.0f, 0.0f, /* Bottom left back. */
		1.0f, 0.0f, 0.0f, /* Bottom right back. */
		0.0f, 0.0f, 1.0f, /* Bottom left front. */
		1.0f, 0.0f, 1.0f, /* Bottom right front. */
		0.0f, 1.0f, 0.0f, /* Top left back. */
		1.0f, 1.0f, 0.0f, /* Top right back. */
		0.0f, 1.0f, 1.0f, /* Top left front. */
		1.0f, 1.0f, 1.0f, /* Top right front. */
	};
	const unsigned char border_lines_indices[] = {
		0u, 1u,  0u, 2u,  1u, 3u,  2u, 3u,
		0u, 4u,  1u, 5u,  2u, 6u,  3u, 7u,
		4u, 5u,  4u, 6u,  5u, 7u,  6u, 7u
	};

	const struct {
		float r, g, b, xmult, ymult, zmult;
		float region_or_chunk; /* 0.0 for chunk pos, 1.0 for region pos. */
	} border_instance_data[] = {
		{
			0.0f, 0.0f, 1.0f,
			VX_WLD_CHUNK_XBLKS, VX_WLD_CHUNK_YBLKS, VX_WLD_CHUNK_ZBLKS,
			0.0f
		},
		{
			1.0f, 0.0f, 0.0f,
			VX_WLD_REGION_XBLKS, VX_WLD_REGION_YBLKS, VX_WLD_REGION_ZBLKS,
			1.0f
		}
	};
	vxctx_init(&vxsd_shaders.border,
		VX_CTX_VBO(border_shape_lines), VX_CTX_VBO(border_instance_data), VX_CTX_EBO(border_lines_indices),
		VX_NULL, vxen_ctxorder_border, vxen_ctxmode_lines,
		&vxtg_toggles.chunk_borders, vxen_ctxcond_uchar, VX_NULL
	);
}
/* Initialize axis indicator. */
static void vxwld_axis_init(VX_NO_ARG)
{
	const struct { float x, y, z, r, g, b; } axis_shape_data[] = {
		{ 0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f }
	};
	vxctx_init(&vxsd_shaders.axis,
		VX_CTX_VBO(axis_shape_data), VX_CTX_NVBO, VX_CTX_NEBO,
		VX_NULL, vxen_ctxorder_axis, vxen_ctxmode_lines,
		&vxtg_toggles.show_any_gui, vxen_ctxcond_uchar, VX_NULL
	);
}

void vxwld_destroy(VX_NO_ARG)
{
	world_active = 0;
	vxwld_resume_threads();
	while (vxwld_gthread.active != -1);

	vxwld_destroy_all_regions();
	vxwld_clear_render_regions();

	for (size_t i = 0u; i < vxwld_mesh_result_queue.size; ++i) {
		vxwld_mesh_result *result = VX_CAST(vxwld_mesh_result *, vxwld_mesh_result_queue.data) + i;
		VX_FREE(result->overall_mesh);
		VX_FREE(result->overall_ebo);
	}
	vxdy_array_free(&vxwld_mesh_result_queue);
	vxdy_array_free(&vxwld_tomesh_queue);
}
