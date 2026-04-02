#pragma once
#ifndef SOURCE_SHADERS_PROGRAMS_VXL_HDR
#define SOURCE_SHADERS_PROGRAMS_VXL_HDR
/* Shader programs header. */

#include "directives/dextern.h"

#include <stddef.h>

/* Normal rendering shader object. */
struct vxstruct_sdr_prog_attribs
{
	unsigned int ogl_type, list_index, loc_index;
	int integral, instanced;
	int stride, count;
};
struct vxstruct_sd_shader_program
{
	const char *name;
	struct vxstruct_sdr_prog_attribs *attributes;
	unsigned int attribs_count;
	unsigned int pid, vertex, fragment;
	size_t base_stride, inst_stride;
};

/* List of normal rendering shaders. */
extern struct vxstruct_sd_shaders_list
{
	struct vxstruct_sd_shader_program
		axis,
		blocks,
		border,
		clouds,
		inventory,
		outline,
		simple,
		sky,
		stars,
		text,
		planets;
} vxsd_shaders;

VX_C_START

/* Create a shader from the filename, loading it into the given shader program object. */
void vxsd_init_shader(
	struct vxstruct_sd_shader_program *sdr_prog,
	char *path, const char *name,
	const char **other_sources, int other_sources_length
);

/* Destroy and deallocate the given shader. */
void vxsd_destroy_shader(struct vxstruct_sd_shader_program *sdr_prog);

/* Uniform variable setters. */

int vxsd_get_location(const struct vxstruct_sd_shader_program *sdr_prog, const char *name);

void vxsd_set_ints(const struct vxstruct_sd_shader_program *sdr_prog, int location, const int *values, int count);
void vxsd_set_floats(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *values, int count);

void vxsd_set_vec2(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *float_vals);
void vxsd_set_vec3(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *float_vals);
void vxsd_set_vec4(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *float_vals);

VX_C_END

#endif
