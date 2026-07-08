#include "directives/dos.h"

#include "values/elements.h"

#include "directives/dsets.h"
#include "directives/dfree.h"
#include "directives/dcast.h"

#include "shaders/textures.h"

#include "utils/png.h"

#include "io/format.h"
#include "io/files.h"
#include "io/logs.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define VX_FILE_ID "elements.c"

#define VX_ELEMENT_ABORT(...) VX_ABORT(vxfmt_text(__VA_ARGS__))
#define VX_ELEMENT_WARN(msg, ...) vxlog_free(VX_LOG_ELEMENT_BIT | VX_LOG_WARNING_BIT, vxfmt_text(msg, __VA_ARGS__))

const wpos vxelm_dirs[6] = {
	{  1,  0,  0  }, /* Right vector. */
	{  0,  1,  0  }, /* Top vector. */
	{  0,  0,  1  }, /* Front vector. */
	{ -1,  0,  0  }, /* Left vector. */
	{  0, -1,  0  }, /* Bottom vector. */
	{  0,  0, -1  }, /* Back vector. */
};

vxelm_mesh *vxelm_meshes;
size_t vxelm_meshes_count;

vxelm_attribs *vxelm_elements;
size_t vxelm_elements_count;

float vxelm_block_texture_y_pixel;

/* Axis direction from letter or -1 on unknown. */
static int vxelm_axis_direction(char axis_letter)
{
	const char lowercase_bit = axis_letter & 32;
	const char minimum = 'X' + lowercase_bit, maximum = minimum + 2;
	if (axis_letter < minimum || axis_letter > maximum) return -1;
	const int start_direction = lowercase_bit ? wdir_left : wdir_right;
	return start_direction + (axis_letter - minimum);
}

static void vxelm_parser_skip_comments(char **parser)
{
	char *p = *parser;
	while (*p && *p == '\n') ++p;

	while (*p && *p == '#') {
		do ++p; while (*p && *p != '\n');
		while (*p && *p == '\n') ++p;
	} 

	*parser = p;
}

static void vxelm_vert_determine_cull(vxelm_mesh *entry, vxelm_mesh_vertex *tri, int dir)
{
	/* Already determined to not cull. */
	if (!entry->cull[dir]) return;
	entry->cull[dir] =
		(tri->x == 0.0f || tri->x == 1.0f) &&
		(tri->y == 0.0f || tri->y == 1.0f) &&
		(tri->z == 0.0f || tri->z == 1.0f);
}

static void vxelm_load_mesh(VX_NO_ARG)
{
	char *verts_file_path = vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &vxfile_exec_dir, "resources" VX_SEPERATOR "elements" VX_SEPERATOR "vertices.txt");
	char *verts_file = vxfile_read(verts_file_path, VX_NULL);
	if (!verts_file) VX_ELEMENT_ABORT("Could not open verts data file '%s'", verts_file_path);

	char *verts_parser = verts_file;
	vxelm_meshes_count = 0u;

	do {
		vxelm_parser_skip_comments(&verts_parser);

		/* Allocate space for another mesh entry. */
		void *mesh_rlc_ptr = realloc(vxelm_meshes, sizeof *vxelm_meshes * ++vxelm_meshes_count);
		if (!mesh_rlc_ptr) VX_ABORT_ALLOCATION();
		else if (vxelm_meshes != mesh_rlc_ptr) vxelm_meshes = VX_CAST(vxelm_mesh *, mesh_rlc_ptr);

		int face_dir = -1;
		char supposed_face_dir_char = '\0';

		/* Determine mesh name. */
		vxelm_mesh *entry = vxelm_meshes + (vxelm_meshes_count - 1u);
		int bytes_read;
		if (sscanf(verts_parser, "@%103s\n%n", entry->name, &bytes_read) != 1 || bytes_read == -1) goto vertex_parse_error;
		entry->name_size = strlen(entry->name);
		verts_parser += bytes_read;

		/* Set cull status to 2 to differentiate from those not being culled (0). */
		for (int i = 0; i < 6; ++i) {
			entry->verts[i] = VX_NULL;
			entry->ebos[i] = VX_NULL;
			entry->verts_cnts[i] = 0;
			entry->ebos_cnts[i] = 0;
			entry->cull[i] = 2;
		}

		do {
			vxelm_parser_skip_comments(&verts_parser);

			/* Stop if the end of the mesh was reached (either EOF or another mesh definition). */
			if (*verts_parser == '@' || !*verts_parser) break;

			/* Determine which axis direction the following vertex data corresponds to.
			   If not, assume this is still vertex data. */
			int is_face_declaration = sscanf(verts_parser, "%%%c %n", &supposed_face_dir_char, &bytes_read);
			if (!is_face_declaration && face_dir == -1) goto vertex_parse_error;
			else if (is_face_declaration) {
				face_dir = vxelm_axis_direction(supposed_face_dir_char);
				verts_parser += bytes_read;

				unsigned int *face_ebos = entry->ebos[face_dir];
				size_t ebos_allocd = 0u;
				char *ebo_end = VX_NULL;
				int *ebo_count = entry->ebos_cnts + face_dir;
	
				/* Get all EBO indices after the face declaration. */
				do {
					while (*verts_parser == ' ') ++verts_parser;
					if (*verts_parser == '\n') break;

					if (VX_CAST(size_t, *ebo_count) >= ebos_allocd) {
						if (ebos_allocd == 0u) ebos_allocd = 6u;
						void *rlc_ebos = realloc(face_ebos, sizeof *face_ebos * (ebos_allocd *= 2u));
						if (!rlc_ebos) VX_ABORT_ALLOCATION();
						else if (face_ebos != rlc_ebos) face_ebos = VX_CAST(uint32_t *, rlc_ebos);
						entry->ebos[face_dir] = face_ebos;
					}

					face_ebos[(*ebo_count)++] = VX_CAST(uint32_t, strtoul(verts_parser, &ebo_end, 0));
					verts_parser = ebo_end;
				} while (1);

				vxelm_parser_skip_comments(&verts_parser);
			}

			vxelm_mesh_vertex tmp;
			/* 'sscanf' deals with spacing, so simply search for 5 floats (3 pos + 2 tex coords).
			   Place into temporary vertex to avoid allocating and then realising it's not a vertex. */
			if (sscanf(verts_parser, "%f%f%f%f%f\n%n", &tmp.x, &tmp.y, &tmp.z, &tmp.u, &tmp.v, &bytes_read) != 5) break;

			void *mesh_rlc = realloc(entry->verts[face_dir], sizeof **entry->verts * VX_CAST(size_t, ++entry->verts_cnts[face_dir]));
			if (!mesh_rlc) VX_ABORT_ALLOCATION();
			else if (entry->verts[face_dir] != mesh_rlc) entry->verts[face_dir] = VX_CAST(vxelm_mesh_vertex *, mesh_rlc);

			if (entry->verts_cnts[face_dir] >= entry->ebos_cnts[face_dir]) {
				VX_ELEMENT_ABORT("%s face %c indices missing", entry->name, supposed_face_dir_char);
			}

			/* Copy results into recently allocated vertex. */
			memcpy(entry->verts[face_dir] + (entry->verts_cnts[face_dir] - 1), &tmp, sizeof tmp);
			verts_parser += bytes_read;

			vxelm_vert_determine_cull(entry, &tmp, face_dir);
		} while (1);

		VX_ELEMENT_LOG(
			1, "+Mesh '%s' VERTS:%d%d%d%d%d%d EBOS:%d%d%d%d%d%d CULL:%d%d%d%d%d%d", entry->name,
			entry->verts_cnts[0], entry->verts_cnts[1], entry->verts_cnts[2], entry->verts_cnts[3], entry->verts_cnts[4], entry->verts_cnts[5],
			entry->ebos_cnts[0], entry->ebos_cnts[1], entry->ebos_cnts[2], entry->ebos_cnts[3], entry->ebos_cnts[4], entry->ebos_cnts[5],
			entry->cull[0], entry->cull[1], entry->cull[2], entry->cull[3], entry->cull[4], entry->cull[5]
		);

		vxelm_parser_skip_comments(&verts_parser);
		if (*verts_parser == '\0') break;
		continue;
	vertex_parse_error:
		VX_ELEMENT_ABORT("Error parsing vertices in %s", entry->name);
	} while (1);

	VX_FREE(verts_file_path);
	VX_FREE(verts_file);
}

/* Set mesh pointer from block entry, using the name. */
static void vxelm_determine_block_mesh(vxelm_attribs *block_entry)
{
	/* Default to first entry. */
	block_entry->mesh = vxelm_meshes;

	for (size_t i = 0u; i < vxelm_meshes_count; ++i) {
		vxelm_mesh *possible_mesh = vxelm_meshes + i;
		if (block_entry->name_size < possible_mesh->name_size) continue;

		/* Check if the block name's ending matches the mesh's name. */
		if (strncmp(block_entry->name + (block_entry->name_size - possible_mesh->name_size), possible_mesh->name, possible_mesh->name_size) == 0) {
			block_entry->mesh = possible_mesh; 
			break;
		}
	}
}

static void vxelm_load_blocks(VX_NO_ARG)
{
	char *block_file_path = vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &vxfile_exec_dir, "resources" VX_SEPERATOR "elements" VX_SEPERATOR "blocks.txt");
	char *block_file = vxfile_read(block_file_path, VX_NULL);
	if (!block_file) VX_ELEMENT_ABORT("Could not open block data file '%s'", block_file_path);

	/* Skip initial comments to set parser start to actual contents. */
	char *number_parser = block_file, *first_entry = VX_NULL;
	vxelm_parser_skip_comments(&number_parser);
	first_entry = number_parser;

	vxelm_elements_count = 0u;
	
	/* Determine the number of entries. */
	do {
		++vxelm_elements_count;
		do ++number_parser; while (*number_parser && *number_parser != '\n');
		vxelm_parser_skip_comments(&number_parser);
	} while (*number_parser);

	vxelm_elements = VX_CAST(vxelm_attribs *, calloc(vxelm_elements_count, sizeof *vxelm_elements));
	if (!vxelm_elements) VX_ABORT_ALLOCATION();

	/* Parse text for all block entries. */
	char *blocks_parser = first_entry;
	vxelm_attribs *entry = vxelm_elements, *const end_entry = entry + vxelm_elements_count;
	do {
		vxelm_parser_skip_comments(&blocks_parser);

		/* Parse entry for name and strength value. */
		int bytes_read = 0;
		const int args_found = sscanf(blocks_parser, "%103s %hd%n\n", entry->name, &entry->strength, &bytes_read);
		if (args_found != 2) VX_ELEMENT_ABORT("Error parsing block entries (block index %d)", VX_CAST(int, entry - vxelm_elements));

		entry->name_size = strlen(entry->name) & 0x7FFu;
		entry->transparency = !(entry->solid = (entry->strength != -1));
		vxelm_determine_block_mesh(entry);

		VX_ELEMENT_LOG(2, "+Block '%s' M:%s S:%d", entry->name, entry->mesh->name, VX_CAST(int, entry->strength));
		blocks_parser += bytes_read;
	} while (*blocks_parser && ++entry != end_entry);

	VX_ELEMENT_LOG(1, "%zu blocks loaded", vxelm_elements_count);

	VX_FREE(block_file_path);
	VX_FREE(block_file);
}

static void vxelm_add_block_texture(char *filename, size_t filename_size, unsigned short tex_index, int is_transparent_texture)
{
	char *original_filename = filename;
	filename[filename_size - 4] = '\0';
	filename_size -= 4u;

	for (size_t i = 1u; i < vxelm_elements_count; ++i) {
		vxelm_attribs *cur_block = vxelm_elements + i;

		/* Determine if the current block has already been filled and if the base names are the same. */
		if (cur_block->face_texs_filled >= 6) continue;
		if (cur_block->name_size > filename_size) continue;
		if (memcmp(cur_block->name, filename, cur_block->name_size) != 0) continue;

		unsigned int texs_filled = cur_block->face_texs_filled;
		unsigned char filled_bitfield = 0u;

		/* Same base names = fill all textures. */
		if (filename_size == cur_block->name_size) {
			for (int t = 0; t < 6; ++t) if (!cur_block->textures[t]) cur_block->textures[t] = tex_index;
			texs_filled = 6u;
			goto skip_name_index_loop;
		}

		/* Determine which texture index to add to from the texture filename's 'axis descriptions'. */
		for (size_t t = cur_block->name_size; t < filename_size; ++t) {
			const int face_ind = vxelm_axis_direction(filename[t]);
			if (filled_bitfield & (1 << face_ind)) continue;
			else if (face_ind == -1) {
				VX_ELEMENT_WARN("Could not determine face from filename %s (char index %zu)", filename, t);
				continue;
			} else if (cur_block->textures[face_ind]) {
				VX_ELEMENT_WARN("Face texture %c already present whilst adding %s to block %s", filename[t], original_filename, cur_block->name);
				continue;
			}

			cur_block->textures[face_ind] = tex_index;
			filled_bitfield |= 1 << face_ind;
			if (++texs_filled >= 6u) break;
		}
	skip_name_index_loop:
		cur_block->face_texs_filled = VX_CAST(unsigned char, texs_filled) & 7u;
		cur_block->transparency |= VX_CAST(unsigned, (is_transparent_texture & 1));
		break;
	}
}


#if VX_UNIX == 1
# include <dirent.h>
# define VX_TEX_DIR "resources/textures/blocks/"
#else
# include <windows.h>
# define VX_TEX_DIR "resources\\textures\\blocks\\*"
#endif

static void vxelm_load_block_textures(VX_NO_ARG)
{
	char *textures_path = vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &vxfile_exec_dir, VX_TEX_DIR);
	if (!textures_path) VX_ABORT_ALLOCATION();
	
	uint8_t *total_pixels = VX_NULL;
	#define VX_TEX_BYTES (VX_TEX_DIMS * VX_TEX_DIMS * 4)
	size_t num_textures = 0u;

	int channels;
	uint32_t width, height;

	vxtex_textures.blocks.channels = 4;
	vxtex_textures.blocks.width = VX_TEX_DIMS;

#if VX_WINDOWS == 1
	WIN32_FIND_DATA found_item;
	HANDLE tex_dir = FindFirstFile(textures_path, &found_item);
	if (tex_dir == INVALID_HANDLE_VALUE) VX_ELEMENT_ABORT("Could not open textures folder %s", textures_path);
	textures_path[strlen(textures_path) - 1] = '\0';

	do {
		if (found_item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		char *filename = found_item.cFileName;
#else
	DIR *tex_dir = opendir(textures_path);
	struct dirent *dir;
	if (!tex_dir) VX_ELEMENT_ABORT("Could not open textures folder %s", textures_path);

	while ((dir = readdir(tex_dir)) != NULL) {
		if (dir->d_type != DT_REG) continue;
		char *filename = dir->d_name;
#endif
		/* Ignore parent and current directory entries and non-PNG files. */
		if (memcmp(filename, "..", 2u) == 0 || memcmp(filename, ".", 1u) == 0) continue;
		const size_t filename_size = strlen(filename);
		if (memcmp(filename + (filename_size - 3u), "png", 3u) != 0) continue;

		char *probably_png = vxfmt_concat_allocd(VX_FMT_CONCAT_DEFAULT, &textures_path, filename); 
		if (!probably_png) VX_ABORT_ALLOCATION();

		uint8_t *local_pixels;
		if (!vxpng_load(probably_png, &local_pixels, &width, &height, &channels)) VX_ELEMENT_ABORT("Failed to load texture %s", probably_png);
		if (channels != 4 || width != VX_TEX_DIMS || height != VX_TEX_DIMS) {
			VX_ELEMENT_LOG(0, "Ignoring %s for block textures (W:%" PRIu32 " H:%" PRIu32 " CHN:%d)", filename, width, height, channels);
			continue;
		}

		VX_FREE(probably_png);

		void *rlc_res = realloc(total_pixels, (num_textures + 1u) * VX_TEX_BYTES);
		if (!rlc_res) VX_ABORT_ALLOCATION();
		else if (rlc_res != total_pixels) total_pixels = VX_CAST(uint8_t *, rlc_res);

		uint8_t *local_pixels_start = total_pixels + (num_textures * VX_TEX_BYTES);
		memcpy(local_pixels_start, local_pixels, VX_TEX_BYTES);
		VX_FREE(local_pixels);

		int has_transparency = 0;
		for (size_t i = 3u; i < VX_TEX_BYTES; i += 4u) {
			if (local_pixels_start[i] == 255) continue;
			has_transparency = 1;
			break;
		}
		VX_ELEMENT_LOG(2, "Applying texture from image %s", filename);
		vxelm_add_block_texture(filename, filename_size, VX_CAST(unsigned short, num_textures), has_transparency);
		++num_textures;
#if VX_WINDOWS == 1
	} while (FindNextFile(tex_dir, &found_item));
	FindClose(tex_dir);
#else
	}
	closedir(tex_dir);
#endif
	VX_FREE(textures_path);

	vxtex_textures.blocks.height = VX_CAST(unsigned int, num_textures) * VX_TEX_DIMS;
	vxtex_load_texture(&vxtex_textures.blocks, total_pixels, 0);
	VX_FREE(total_pixels);

	vxelm_block_texture_y_pixel = 1.0f / (VX_CAST(float, num_textures) * VX_TEX_DIMS);
}

void vxelm_load(VX_NO_ARG)
{
	vxelm_load_mesh();
	vxelm_load_blocks();
	vxelm_load_block_textures();
}

void vxelm_destroy(VX_NO_ARG)
{
	VX_FREE(vxelm_elements);

	for (size_t i = 0; i < vxelm_meshes_count; ++i) {
		vxelm_mesh *mesh = vxelm_meshes + i;
		for (int t = 0; t < 6; ++t) {
			VX_FREE(mesh->verts[t]);
			VX_FREE(mesh->ebos[t]);
		}
	}

	VX_FREE(vxelm_meshes);
}
