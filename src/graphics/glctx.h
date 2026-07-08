#pragma once
#ifndef SOURCE_GRAPHICS_CONTEXT_VXL_HDR
#define SOURCE_GRAPHICS_CONTEXT_VXL_HDR
/* Drawing context functions. */

#include "directives/dword.h"

#include "shaders/programs.h"

#include <stddef.h>

enum
{
	vxen_ctxmode_points = 0,
	vxen_ctxmode_lines = 1,
	vxen_ctxmode_tris = 4,
	vxen_ctxmode_tristrips = 5
};
enum
{
	vxen_ctxcond_positivefloat,
	vxen_ctxcond_uchar
};
enum /* Higher value = higher priority. */
{
	vxen_ctxorder_stars,
	vxen_ctxorder_skybox,
	vxen_ctxorder_planets,
	vxen_ctxorder_clouds,
	vxen_ctxorder_world,
	vxen_ctxorder_outline,
	vxen_ctxorder_border,
	vxen_ctxorder_axis,
	vxen_ctxorder_inventory,
	vxen_ctxorder_text,
	vxen_ctxorder_text_inv
};

#define VX_CTX_EBO(arr) arr, sizeof arr, sizeof *arr, VX_NULL
#define VX_CTX_VBO(arr) arr, sizeof arr, VX_NULL

#define VX_CTX_BEBO(arr, sz) arr, sz, sizeof *arr, VX_NULL
#define VX_CTX_BVBO(arr, sz) arr, sz, VX_NULL

#define VX_CTX_NVBO VX_NULL, 0u, VX_NULL
#define VX_CTX_NEBO VX_NULL, 0u, 0, VX_NULL

VX_C_START

/* Initializes a rendering context with the given arguments.
   Handles attributes and picks the most appropriate draw function.
   Returns the context data pointer. */
void *vxctx_init(
	const struct vxstruct_sd_shader_program *sdr_prog,
	const void *base_data, size_t base_bytes, void **get_base_buffer,
	const void *instanced_data, size_t instanced_bytes, void **get_ivbo_buffer,
	const void *ebo_data, size_t ebo_bytes, int ebo_element_size, void **get_ebo_buffer,
	void **get_main_buffer, int priority, unsigned int primitive,
	void *conditional, int conditional_type, int (*frame_update_func)(void *)
);

/* Enable shader attributes and set attrib pointers. */
void vxctx_apply_attribs(const struct vxstruct_sd_shader_program *shader, int pointers_only, int instanced_attribs_only);

/* Either fully reallocate a buffer's data or overwrite an existing section. */
void vxctx_update_buffer(void *buf_info, const void *data, size_t offset, size_t bytes);

/* Returns whether two draw values refer to the same buffer. */
int vxctx_equal_draws(const void *draw_a, const void *draw_b);

/* Draw all contexts. */
void vxctx_draw_all(VX_NO_ARG);

/* Destroy the given buffer. */
void vxctx_destroy_buf(void *buf);
/* Destroy the given context. */
void vxctx_destroy_ctx(void *ctx);
/* Destroy all created contexts. */
void vxctx_destroy_all(VX_NO_ARG);

VX_C_END

#endif
