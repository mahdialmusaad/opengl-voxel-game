#include "shaders/loader.h"
#include "shaders/programs.h"
#include "shaders/textures.h"
#include "shaders/ubo.h"

#include "directives/dcast.h"
#include "directives/dfree.h"
#include "directives/dsets.h"

#include "graphics/glfuncs.h"
#include "graphics/glenum.h"

#include "utils/png.h"

#include "io/files.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VX_FILE_ID "loader.c"

#define VX_TEXTURES_COUNT (sizeof(struct vxstruct_texture_info_list) / sizeof(vxtexture_info))
#define VX_SHADERS_COUNT (sizeof(struct vxstruct_sd_shaders_list) / sizeof(struct vxstruct_sd_shader_program))
#define VX_UNIFORMS_COUNT (sizeof vxubo_bases / sizeof *vxubo_bases)

void vxsd_destroy(VX_NO_ARG)
{
	vxtexture_info *textures = VX_REINT_CAST(vxtexture_info *, &vxtex_textures);
	struct vxstruct_sd_shader_program *shaders = VX_REINT_CAST(struct vxstruct_sd_shader_program *, &vxsd_shaders);

	VX_SHADER_LOG(1, "%zu textures + %zu shaders + %zu ubos bye", VX_TEXTURES_COUNT, VX_SHADERS_COUNT, VX_UNIFORMS_COUNT);

	for (size_t i = 0; i < VX_TEXTURES_COUNT; ++i) vxtex_unload_texture(textures + i);
	for (size_t i = 0; i < VX_UNIFORMS_COUNT; ++i) gl.DeleteBuffers(1, &vxubo_bases[i]->id);
	for (size_t i = 0; i < VX_SHADERS_COUNT; ++i) {
		gl.DeleteProgram(shaders[i].pid);
		VX_FREE(shaders[i].attributes);
		memset(shaders + i, 0, sizeof *shaders);
	}
}

void vxsd_init_all(VX_NO_ARG)
{
	/* One for each UBO and texture, one for version header (first line always) and last for actual shader. */
	const char *all_sources[VX_UNIFORMS_COUNT + VX_TEXTURES_COUNT + 2];

	int sources_index = 0;
	all_sources[sources_index++] = "#version 330 core\n";

	/* Initialize all UBOs. */
	for (GLuint i = 0u, last = VX_UNIFORMS_COUNT; i < last; ++i) {
		struct vxstruct_ubo_base_instance *ubo = vxubo_bases[i];
		all_sources[sources_index++] = ubo->glsl;

		gl.GenBuffers(1, &ubo->id);
		gl.BindBuffer(GL_UNIFORM_BUFFER, ubo->id);
		gl.BindBufferBase(GL_UNIFORM_BUFFER, ubo->id, ubo->id);
		gl.BufferData(GL_UNIFORM_BUFFER, ubo->bytes, VX_NULL, GL_STATIC_DRAW);

		VX_SHADER_LOG(2, "+UBO %s ID:%u", ubo->ubo_name + 4, ubo->id);
	}

	/* Get relevant directories from executable path. */
	char *gui_textures_dir = vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &vxfile_exec_dir, "resources/textures/gui/");
	char *shaders_dir = vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &vxfile_exec_dir, "resources/shaders/");

	vxtexture_info *textures = VX_REINT_CAST(vxtexture_info *, &vxtex_textures);
	if (!gui_textures_dir || !shaders_dir) VX_ABORT_ALLOCATION();

	/* Load other textures (any unloaded from textures list). */
	for (int i = 0, last = VX_TEXTURES_COUNT; i < last; ++i) {
		vxtexture_info *texture = textures + i;
		all_sources[sources_index++] = texture->glsl;
		if (texture->internal_index != -1) continue; /* Loaded elsewhere. */

		char *full_path = vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &gui_textures_dir, texture->filename);
		if (!full_path) goto texture_load_fail;

		uint8_t *pixels;
		if (!vxpng_load(full_path, &pixels, &texture->width, &texture->height, &texture->channels)) goto texture_load_fail;
		if (!vxtex_load_texture(textures + i, pixels, 0)) goto texture_load_fail;

		VX_FREE(pixels);
		VX_FREE(full_path);

		continue;
	texture_load_fail:
		VX_ABORT(vxfmt_text("Failed to load texture %s", full_path));
	}

	VX_FREE(gui_textures_dir);
	int shaders_count = 0;

	/* Preferably the shader file should have the same name as the variable. */
	#define VX_LOAD_SHADER(name) vxsd_init_shader(&vxsd_shaders.name,\
		vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &shaders_dir, #name ".glsl"),\
		#name, all_sources, sizeof all_sources / sizeof *all_sources\
	); ++shaders_count

	VX_LOAD_SHADER(axis);
	VX_LOAD_SHADER(blocks);
	VX_LOAD_SHADER(border);
	VX_LOAD_SHADER(clouds);
	VX_LOAD_SHADER(inventory);
	VX_LOAD_SHADER(outline);
	VX_LOAD_SHADER(simple);
	VX_LOAD_SHADER(sky);
	VX_LOAD_SHADER(stars);
	VX_LOAD_SHADER(text);
	VX_LOAD_SHADER(planets);
	#undef VX_LOAD_SHADER

	VX_SHADER_LOG(1, "+%d shaders, %zu textures and %zu ubos", shaders_count, VX_TEXTURES_COUNT, VX_UNIFORMS_COUNT);
	VX_FREE(shaders_dir);
}
