#include "shaders/textures.h"

#include "directives/dcast.h"
#include "directives/dword.h"
#include "directives/dsets.h"

#include "graphics/glfuncs.h"
#include "graphics/glenum.h"

#include <stdint.h>

struct vxstruct_texture_info_list vxtex_textures = {
#define VX_TEXTURE(name) { (#name ".png"), ("TX_" #name), ("uniform sampler2D TX_" #name ";"), 0.0f, 0.0f, 0u, 0u, 0, 0u, -1, 0 },
#define VX_TEXTURE_EMPTY(name) { VX_NULL, ("TX_" #name), ("uniform sampler2D TX_" #name ";"), 0.0f, 0.0f, 0u, 0u, 0, 0u, -1, 0 },
	VX_TEXTURE_EMPTY(blocks)
	VX_TEXTURE(inventory)
	VX_TEXTURE(text)
#undef  VX_TEXTURE
#undef  VX_TEXTURE_EMPTY
};

int vxtex_load_texture(vxtexture_info *tex_info, void *pixels, int do_mipmap)
{
	if (!pixels) return 0;
	const int is_reload = tex_info->internal_index != -1;
	
	static int current_index = 0;
	if (!is_reload) tex_info->internal_index = current_index++;

	tex_info->pixel_width = 1.0f / VX_CAST(float, tex_info->width);
	tex_info->pixel_height = 1.0f / VX_CAST(float, tex_info->height);

	/* Create the texture and set it as the currently bound one. */
	if (!is_reload) gl.GenTextures(1, &tex_info->bind_id);
	gl.ActiveTexture(GL_TEXTURE0 + VX_CAST(GLenum, tex_info->internal_index));
	gl.BindTexture(GL_TEXTURE_2D, tex_info->bind_id);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, do_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	const GLenum format = (tex_info->channels == 1) ? GL_RED : ((tex_info->channels == 3) ? GL_RGB : GL_RGBA);

	/* Save the given texture data into the currently bound texture object. */
	gl.TexImage2D(
		GL_TEXTURE_2D, 0, VX_CAST(GLint, format),
		VX_CAST(GLsizei, tex_info->width), VX_CAST(GLsizei, tex_info->height),
		0, VX_CAST(GLenum, format),
		GL_UNSIGNED_BYTE, pixels
	);
	if (do_mipmap) gl.GenerateMipmap(GL_TEXTURE_2D);

	VX_SHADER_LOG(
		1, "%s %s W:%u H:%u CHN:%u ID:%u", is_reload ? "Reloading tex" : "+Texture",
		tex_info->tex_name + 3, tex_info->width, tex_info->height, tex_info->channels, tex_info->bind_id
	);
	return 1;
}

void vxtex_unload_texture(vxtexture_info *tex_info)
{
	VX_SHADER_LOG(2, "-Texture %s ID:%u", tex_info->tex_name + 3, tex_info->bind_id);
	gl.DeleteTextures(1, &tex_info->bind_id);
	tex_info->internal_index = -1;
}
