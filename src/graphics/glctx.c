#include "graphics/glctx.h"
#include "graphics/glfuncs.h"
#include "graphics/glenum.h"

#include "directives/dcast.h"
#include "directives/dword.h"
#include "directives/dfree.h"
#include "directives/dsets.h"

#include "utils/dyarray.h"

#include "io/logs.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define VX_FILE_ID "glctx.c"

/* Per-frame update function for a context. */
typedef int (*vxctx_frame_func)(void *ordered_draw_info);

/* Context buffer data. */
typedef struct vxctx_buffer
{
	struct vxgl_context *ctx;
	size_t allocd;

	GLuint id;
	GLsizei local_count;

	int priority;
	int type : 31;
	unsigned int sharing : 1;
} vxctx_buffer;
/* Rendering context for some data. */
typedef struct vxgl_context
{
	const struct vxstruct_sd_shader_program *shader;
	vxctx_frame_func frame_update_func;
	void *condition_check;
	
	vxctx_buffer *vbos;
	int condition_type;
	unsigned int vbos_count;
	
	size_t sizeof_index;
	vxctx_buffer ebo;
	GLuint vao;

	GLsizei base_primitives_count;
	
	int draw_func;
	unsigned int draw_mode;
} vxctx_context;

enum
{
	vxen_ctxfunc_instanced = 1,
	vxen_ctxfunc_elements = 2,

	vxen_ctxfunc_drawarrays = 0,
	vxen_ctxfunc_drawarraysinst = vxen_ctxfunc_instanced,
	vxen_ctxfunc_drawelements = vxen_ctxfunc_elements,
	vxen_ctxfunc_drawelementsinst = vxen_ctxfunc_instanced | vxen_ctxfunc_elements
};
enum
{
	vxen_ctxbuf_ebo,
	vxen_ctxbuf_vbo,
	vxen_ctxbuf_ivbo
};

/* Persistent draw information. */
typedef struct vxctx_ordered_draw
{
	vxctx_context *ctx;
	unsigned int vbo_index;
	int type;
} vxctx_ordered_draw;

static vxdy_array vxctx_identifiers_list = VX_DYARRAY_INIT(sizeof(vxctx_ordered_draw *), 0);
static vxdy_array vxctx_contexts_list = VX_DYARRAY_INIT(sizeof(vxctx_context *), 0);
static vxdy_array vxctx_draws_list = VX_DYARRAY_INIT(sizeof(vxctx_ordered_draw), 0);
static unsigned char vxctx_in_draw = 0u;

static GLenum vxctx_ogltype(int buf_type)
{
	if (buf_type == vxen_ctxbuf_ebo) return GL_ELEMENT_ARRAY_BUFFER;
	else return GL_ARRAY_BUFFER;
}
static vxctx_buffer *vxctx_buf_from_draw(const vxctx_ordered_draw *draw)
{
	if (draw->type == vxen_ctxbuf_ebo) return &draw->ctx->ebo;
	else return draw->ctx->vbos + draw->vbo_index;
}
static int vxctx_cmp_draws(const void *draw_void_a, const void *draw_void_b)
{
	const vxctx_ordered_draw *draw_a = VX_CAST(const vxctx_ordered_draw *, draw_void_a);
	const vxctx_ordered_draw *draw_b = VX_CAST(const vxctx_ordered_draw *, draw_void_b);
	return vxctx_buf_from_draw(draw_a)->priority - vxctx_buf_from_draw(draw_b)->priority;
}

static vxctx_ordered_draw *vxctx_create_drawable(vxctx_buffer *buf, vxctx_ordered_draw *draw)
{
	draw->ctx = buf->ctx;
	draw->type = buf->type;
	draw->vbo_index = (buf->type != vxen_ctxbuf_ebo) ? VX_CAST(unsigned int, buf - buf->ctx->vbos) : 0u;
	return draw;
}


static void vxctx_add_drawable(vxctx_buffer *buf)
{
	vxctx_ordered_draw draw_info;
	vxctx_create_drawable(buf, &draw_info);
	vxdy_array_add(&vxctx_draws_list, &draw_info);
	qsort(vxctx_draws_list.data, vxctx_draws_list.size, sizeof(vxctx_ordered_draw), vxctx_cmp_draws);
}

static void vxctx_update_count(vxctx_buffer *buf, size_t bytes_used)
{
	switch (buf->type) {
		default: break;
		case vxen_ctxbuf_vbo:
			if (buf->ctx->sizeof_index) break;
			buf->ctx->base_primitives_count = VX_CAST(GLsizei, bytes_used / buf->ctx->shader->base_stride);
			break;
		case vxen_ctxbuf_ebo:
			buf->ctx->base_primitives_count = VX_CAST(GLsizei, bytes_used / buf->ctx->sizeof_index);
			break;
		case vxen_ctxbuf_ivbo:
			buf->local_count = VX_CAST(GLsizei, bytes_used / buf->ctx->shader->inst_stride);
			break;
	}
}

void vxctx_apply_attribs(const struct vxstruct_sd_shader_program *shader, int pointers_only, int instanced_attribs_only)
{
	const GLsizei total_stride = (shader->attribs_count == 1) ? 0 : VX_CAST(GLsizei, instanced_attribs_only ? shader->inst_stride : shader->base_stride);
	const struct vxstruct_sdr_prog_attribs *attrib = VX_CAST(const struct vxstruct_sdr_prog_attribs *, shader->attributes);

	for (size_t i = 0u; i < shader->attribs_count; ++i, ++attrib) {
		if (instanced_attribs_only != attrib->instanced) continue;

		const void *local_stride = VX_REINT_CAST(const void *, VX_CAST(size_t, attrib->stride));
		if (attrib->integral) gl.VertexAttribIPointer(attrib->loc_index, attrib->count, attrib->ogl_type, total_stride, local_stride);
		else gl.VertexAttribPointer(attrib->loc_index, attrib->count, attrib->ogl_type, 0, total_stride, local_stride);

		if (pointers_only) continue;
		if (instanced_attribs_only) gl.VertexAttribDivisor(attrib->loc_index, 1u);
		gl.EnableVertexAttribArray(attrib->loc_index);
	}
}

/* Enable and set attributes present in the given shader to the given VBO. */
static void vxctx_apply_attribs_internal(const vxctx_buffer *buf)
{
	if (buf->type == vxen_ctxbuf_ebo) return;
	vxctx_apply_attribs(buf->ctx->shader, buf->sharing, buf->type == vxen_ctxbuf_ivbo);
}

static vxctx_buffer *vxctx_init_buffer(vxctx_context *ctx, vxctx_buffer *buf, const void *data, size_t bytes, int priority, int type)
{
	buf->priority = priority;
	buf->local_count = 0;
	buf->allocd = bytes;
	buf->type = type & 0xFF;
	buf->sharing = 0;
	buf->ctx = ctx;

	const GLenum ogl_type = vxctx_ogltype(type);

	gl.GenBuffers(1, &buf->id);
	gl.BindBuffer(ogl_type, buf->id);
	gl.BufferData(ogl_type, VX_CAST(GLsizeiptr, bytes), data, GL_STATIC_DRAW);

	vxctx_apply_attribs_internal(buf);
	if (!data) bytes = 0u;
	vxctx_update_count(buf, bytes);

	return buf;
}

static vxctx_buffer *vxctx_alloc_buf(vxctx_context *ctx)
{
	vxctx_buffer *vbos_data = VX_CAST(vxctx_buffer *, realloc(ctx->vbos, sizeof *vbos_data * ++ctx->vbos_count));
	if (!vbos_data) VX_ABORT_ALLOCATION();
	if (vbos_data != ctx->vbos) ctx->vbos = vbos_data;
	return vbos_data + (ctx->vbos_count - 1u);
}

static vxctx_ordered_draw *vxctx_alloc_identifier(vxctx_buffer *buf)
{
	vxctx_ordered_draw *id_ptr = VX_CAST(vxctx_ordered_draw *, malloc(sizeof *id_ptr));
	if (!id_ptr) VX_ABORT_ALLOCATION();
	vxdy_array_add(&vxctx_identifiers_list, &id_ptr);
	return vxctx_create_drawable(buf, id_ptr);
}

void *vxctx_init(
	const struct vxstruct_sd_shader_program *sdr_prog,
	const void *base_data, size_t base_bytes, void **get_base_buffer,
	const void *instanced_data, size_t instanced_bytes, void **get_ivbo_buffer,
	const void *ebo_data, size_t ebo_bytes, int ebo_element_size, void **get_ebo_buffer,
	void **get_main_buffer, int priority, unsigned int primitive,
	void *conditional, int conditional_type, vxctx_frame_func frame_update_func
) {
	vxctx_context *ctx = VX_CAST(vxctx_context *, malloc(sizeof *ctx));
	if (!ctx || !vxdy_array_add(&vxctx_contexts_list, &ctx)) VX_ABORT_ALLOCATION();

	gl.GenVertexArrays(1, &ctx->vao);
	gl.BindVertexArray(ctx->vao);

	ctx->vbos_count = 0u;
	ctx->vbos = VX_NULL;

	ctx->shader = sdr_prog;
	ctx->draw_mode = VX_CAST(unsigned int, primitive);
	ctx->sizeof_index = VX_CAST(size_t, ebo_element_size);
	ctx->frame_update_func = frame_update_func;
	ctx->condition_check = conditional;
	ctx->condition_type = conditional_type;
	
	const int has_instanced = instanced_bytes != 0;
	const int has_elements = ebo_bytes != 0;
	const int has_base = base_bytes != 0;
	ctx->draw_func = has_instanced | (has_elements * 2);
	
	vxctx_buffer *drawable_target = VX_NULL, *base_buffer = VX_NULL, *ivbo_buffer = VX_NULL, *ebo_buffer = VX_NULL;
	if (has_base) drawable_target = base_buffer = vxctx_init_buffer(ctx, vxctx_alloc_buf(ctx), base_data, base_bytes, priority, vxen_ctxbuf_vbo);
	if (has_elements) drawable_target = ivbo_buffer = vxctx_init_buffer(ctx, &ctx->ebo, ebo_data, ebo_bytes, priority, vxen_ctxbuf_ebo);
	if (has_instanced) drawable_target = ebo_buffer = vxctx_init_buffer(ctx, vxctx_alloc_buf(ctx), instanced_data, instanced_bytes, priority, vxen_ctxbuf_ivbo);
	if (drawable_target) vxctx_add_drawable(drawable_target);

	if (get_main_buffer && drawable_target) *get_main_buffer = vxctx_alloc_identifier(drawable_target);
	if (get_base_buffer && base_buffer) *get_base_buffer = vxctx_alloc_identifier(base_buffer);
	if (get_ebo_buffer && ebo_buffer) *get_ebo_buffer = vxctx_alloc_identifier(ebo_buffer);
	if (get_ivbo_buffer && ivbo_buffer) *get_ivbo_buffer = vxctx_alloc_identifier(ivbo_buffer);

	return ctx;
}

void vxctx_update_buffer(void *buf_info, const void *data, size_t offset, size_t bytes)
{
	vxctx_buffer *buf = vxctx_buf_from_draw(VX_CAST(vxctx_ordered_draw *, buf_info));
	const GLenum ogl_type = vxctx_ogltype(buf->type);

	if (!vxctx_in_draw) gl.BindVertexArray(buf->ctx->vao);
	gl.BindBuffer(ogl_type, buf->id);

	vxctx_update_count(buf, bytes);

	/* Allocate space for buffer if not large enough. */
	if (bytes + offset > buf->allocd) {
		gl.BufferData(ogl_type, VX_CAST(GLsizeiptr, bytes), data, GL_STATIC_DRAW);
		buf->allocd = bytes;
	} else gl.BufferSubData(ogl_type, VX_CAST(GLintptr, offset), VX_CAST(GLsizeiptr, bytes), data);
}


int vxctx_equal_draws(const void *draw_vda, const void *draw_vdb)
{
	const vxctx_ordered_draw *draw_a = VX_CAST(const vxctx_ordered_draw *, draw_vda);
	const vxctx_ordered_draw *draw_b = VX_CAST(const vxctx_ordered_draw *, draw_vdb);
	return memcmp(draw_a, draw_b, sizeof *draw_a) == 0;
}


static void vxctx_handle_draw(vxctx_ordered_draw *draw)
{
	vxctx_buffer *buf = vxctx_buf_from_draw(draw);

	static GLuint prev_pid = 0u, prev_vao = 0u;

	const int update_result = !buf->ctx->frame_update_func ? -1 : buf->ctx->frame_update_func(draw);
	if (update_result == 0) return;
	else if (update_result == 2) {
		prev_vao = 0u;
		prev_pid = 0u;
		return;
	}

	const GLsizei base_count = buf->ctx->base_primitives_count;
	if (!base_count) return;
	if ((buf->ctx->draw_func & vxen_ctxfunc_instanced) && !buf->local_count) return;

	/* Use the linked shader program and VAO for drawing. */
	if (draw->ctx->shader->pid != prev_pid) gl.UseProgram(prev_pid = draw->ctx->shader->pid);
	if (draw->ctx->vao != prev_vao) gl.BindVertexArray(prev_vao = draw->ctx->vao);

	/* Get OpenGL's corresponding values from the context's variables. */
	const GLenum ebo_type = GL_UNSIGNED_BYTE + (buf->ctx->sizeof_index == 1 ? 0u : VX_CAST(GLenum, buf->ctx->sizeof_index));
	const GLenum draw_mode = buf->ctx->draw_mode;

	if (buf->type != vxen_ctxbuf_ebo && buf->ctx->vbos_count > 1u && buf->ctx->vbos[0].type == buf->ctx->vbos[1].type) {
		gl.BindBuffer(GL_ARRAY_BUFFER, buf->id);
		buf->sharing = 1u;
		vxctx_apply_attribs_internal(buf);
		buf->sharing = 0u;
	}

	/* Use the corresponding draw function. */
	switch (buf->ctx->draw_func) {
		default: break;
		case vxen_ctxfunc_drawarrays:
			gl.DrawArrays(draw_mode, 0, base_count);
			break;
		case vxen_ctxfunc_drawarraysinst:
			gl.DrawArraysInstanced(draw_mode, 0, base_count, buf->local_count);
			break;
		case vxen_ctxfunc_drawelements:
			gl.DrawElements(draw_mode, base_count, ebo_type, VX_NULL);
			break;
		case vxen_ctxfunc_drawelementsinst:
			gl.DrawElementsInstanced(draw_mode, base_count, ebo_type, VX_NULL, buf->local_count);
			break;
	}
}
/* Check if the context's draw condition passes, if there is one. */
static int vxctx_check_condition(vxctx_context *ctx)
{
	if (!ctx->condition_check) return 1;
	switch (ctx->condition_type) {
		default: return 1;
		case vxen_ctxcond_positivefloat:
			if (*VX_CAST(float *, ctx->condition_check) <= 0.0f) return 0;
			return 1;
		case vxen_ctxcond_uchar:
			if (*VX_CAST(unsigned char *, ctx->condition_check) == 0) return 0;
			return 1;
	}
}

void vxctx_draw_all(VX_NO_ARG)
{
	vxctx_in_draw = 1u;

	for (size_t i = 0u; i < vxctx_draws_list.size; ++i) {
		vxctx_ordered_draw *draw = VX_CAST(vxctx_ordered_draw *, vxctx_draws_list.data) + i;
		if (!vxctx_check_condition(draw->ctx)) continue;
		vxctx_handle_draw(draw);
	}

	vxctx_in_draw = 0u;
}


void vxctx_destroy_buf(void *buf_info)
{
	vxctx_buffer *buf = vxctx_buf_from_draw(VX_CAST(vxctx_ordered_draw *, buf_info));
	unsigned int vbos_count = buf->ctx->vbos_count;
	
	--buf->ctx->vbos_count;
	gl.DeleteBuffers(1, &buf->id);

	/* Overwrite given buffer with adjacent buffers. */
	for (unsigned int i = 0; i < vbos_count; ++i) {
		if (buf->ctx->vbos->id != buf->id) continue;
		memmove(
			buf->ctx->vbos + i,
			buf->ctx->vbos + i + 1,
			sizeof *buf * (vbos_count - (i + 1))
		);
		return;
	}
}

static void vxctx_destroy_ctx_impl(vxctx_context *ctx, size_t index, int slide)
{
	if (!ctx->vao) return;
	gl.DeleteVertexArrays(1, &ctx->vao);

	if (ctx->sizeof_index) gl.DeleteBuffers(1, &ctx->ebo.id);
	for (size_t vbo_ind = 0u; vbo_ind < ctx->vbos_count; ++vbo_ind) gl.DeleteBuffers(1, &ctx->vbos[vbo_ind].id);

	VX_FREE(ctx->vbos);
	
	if (slide) {
		vxdy_array_remove(&vxctx_contexts_list, index, 1u);
		VX_CONTEXT_LOG(2, "-ctx %s (%u)", ctx->shader->name, ctx->vao);
	}

	VX_FREE(ctx);
}

void vxctx_destroy_ctx(void *ctx_void)
{
	vxctx_context *ctx = VX_CAST(vxctx_context *, ctx_void);

	/* Search the 'draws' list for any that use this context and remove them. */
	for (size_t i = 0u; i < vxctx_draws_list.size; ++i) {
		vxctx_ordered_draw *draw = VX_CAST(vxctx_ordered_draw *, vxctx_draws_list.data) + i;
		if (draw->ctx->vao == ctx->vao) vxdy_array_remove(&vxctx_draws_list, i--, 1u);
	}

	/* Search for the index of the given context and destroy it. */
	for (size_t i = 0u; i < vxctx_contexts_list.size; ++i) {
		if (VX_CAST(vxctx_context **, vxctx_contexts_list.data)[i]->vao != ctx->vao) continue;
		vxctx_destroy_ctx_impl(ctx, i, 1);
		break;
	}
}

void vxctx_destroy_all(VX_NO_ARG)
{
	VX_CONTEXT_LOG(1, "%zu ctxs bye", vxctx_contexts_list.size, vxctx_contexts_list.size);

	for (size_t i = 0u; i < vxctx_contexts_list.size; ++i) vxctx_destroy_ctx_impl(VX_CAST(vxctx_context **, vxctx_contexts_list.data)[i], 0u, 0);
	for (size_t i = 0u; i < vxctx_identifiers_list.size; ++i) VX_FREE(VX_CAST(vxctx_ordered_draw **, vxctx_identifiers_list.data)[i]);

	vxdy_array_free(&vxctx_identifiers_list);
	vxdy_array_free(&vxctx_contexts_list);
	vxdy_array_free(&vxctx_draws_list);
}
