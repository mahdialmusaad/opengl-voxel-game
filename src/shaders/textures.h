#pragma once
#ifndef SOURCE_SHADERS_TEXTURES_VXL_HDR
#define SOURCE_SHADERS_TEXTURES_VXL_HDR
/* Shader textures information. */

#include "directives/dextern.h"

/* Texture object information. */
typedef struct
{
	const char *filename, *tex_name, *glsl;
	float pixel_width, pixel_height;
	unsigned int width, height;
	unsigned int bind_id;
	int channels;
	int internal_index;
	int free;
} vxtexture_info;

/* All textures information. */
extern struct vxstruct_texture_info_list
{
	vxtexture_info blocks, inventory, text;
} vxtex_textures;

VX_C_START

/* Load a texture for rendering usage.
   The given texture object should have the width, height and channel members set. */
int vxtex_load_texture(vxtexture_info *tex_info, void *pixels, int do_mipmap);
/* Unload the given texture and deallocate related resources. */
void vxtex_unload_texture(vxtexture_info *tex_info);

#endif
