#pragma once
#ifndef SOURCE_SHADERS_UBO_VXL_HDR
#define SOURCE_SHADERS_UBO_VXL_HDR

/* Shader uniform objects. */

#include "directives/dextern.h"

#include "vector/mat4.h"
#include "vector/vec3.h"

#include <stddef.h>

/* Shader UBO base instance. */
struct vxstruct_ubo_base_instance
{
	const char *glsl, *ubo_name;
	unsigned int id;
	unsigned int bytes;
};

struct vxstruct_ubo_vector4 { vec3 v; float w; };

#define ubo_v4 struct vxstruct_ubo_vector4

#if defined(__cplusplus)
# define VX_UBO_INTERNAL_DATA(type, first, ...) union { type first, __VA_ARGS__; }
#else
# define VX_UBO_INTERNAL_DATA(type, first, ...) type first, __VA_ARGS__
#endif

/* Declare UBO struct and variables. */
#define VX_UBO_CREATE(name, glsl_type_name, type, init, first, ...)\
struct vxstruct_ubo_##name##_object {\
	struct vxstruct_ubo_base_instance base;\
	const size_t data_start_offset;\
	VX_UBO_INTERNAL_DATA(type, first, __VA_ARGS__);\
} name;

#define VX_UBO_ADDITION(...) + 1

/* All UBO declarations. They are automatically defined in the source.
   Use the 'operation' parameter and provide the arguments shown in VX_UBO_CREATE. */
#define VX_UBO_MASTER(operation)\
/* ****** You can add or change UBO variables here, as well as create UBOs. ****** */\
operation(mat4s, mat4, mat4, { 0 }, M4_camera,M4_origin,M4_stars,M4_planets,M4_axis)\
operation(floats, float, float, 0.0f, FLT_ctime,FLT_fog_end,FLT_fog_range,FLT_stars_trnsp,FLT_gtime,FLT_clouds_col,FLT_aspect,FLT_padding)\
operation(vec4s, vec4, ubo_v4, { 0 }, V4_main_sky,V4_evening_sky,V4_raycast_lpos,V4_chunk_lpos,V4_region_lpos,V4_world_light,V4_clouds_offset)

extern struct vxstruct_ubo_list { VX_UBO_MASTER(VX_UBO_CREATE) } vxubo_list;
extern struct vxstruct_ubo_base_instance *vxubo_bases[VX_UBO_MASTER(VX_UBO_ADDITION)];

#undef ubo_v4
#undef VX_UBO_CREATE
#undef VX_UBO_ADDITION
#undef VX_UBO_SET_DATA

/* Update the UBO to use the values given. Use the provided VX_UBO* macros instead. */
VX_C_FUNC void vxubo_update_direct(struct vxstruct_ubo_base_instance *ubo_inst, const void *data, size_t offset, size_t bytes);

/* Buffer the current UBO data. */
#define VX_UBO_UPDATE(name) vxubo_update_direct(\
	&vxubo_list.name.base,\
	VX_REINT_CAST(const unsigned char *, &vxubo_list.name) + vxubo_list.name.data_start_offset,\
	0u,\
	sizeof(vxubo_list.name) - vxubo_list.name.data_start_offset\
)

#endif
