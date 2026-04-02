#include "shaders/programs.h"

#include "shaders/textures.h"
#include "shaders/ubo.h"

#include "directives/dcast.h"
#include "directives/dword.h"
#include "directives/dfree.h"
#include "directives/dsets.h"

#include "graphics/glenum.h"
#include "graphics/glfuncs.h"

#include "io/format.h"
#include "io/files.h"
#include "io/logs.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define VX_FILE_ID "programs.c"

#define VX_SHADER_WARN(msg, ...) vxlog_free(VX_LOG_SHADERS_BIT | VX_LOG_WARNING_BIT, vxfmt_text(msg, __VA_ARGS__))
#define VX_SHADER_ABORT(msg, ...) VX_ABORT(vxfmt_text(msg, __VA_ARGS__))

/* Attribute size and name data. */
typedef struct vxgl_attrib_pointer_data
{
	const char *name;
	GLenum gl_type;
	GLint bytes_each;
	GLint count;
	GLint vectors;
} vxgl_attrib_pointer_data;

#define GL_FLOATING_PNT GL_FLOAT

/* Data sizes and name of attribute types. */
static const vxgl_attrib_pointer_data vxsd_attrib_types[] = {
	{ "float", GL_FLOATING_PNT, sizeof(GLfloat), 1, 1 },
	{ "uint",  GL_UNSIGNED_INT, sizeof(GLuint),  1, 1 },

	{ "vec2",  GL_FLOATING_PNT, sizeof(GLfloat), 2, 1 }, { "vec3",  GL_FLOATING_PNT, sizeof(GLfloat), 3, 1 }, { "vec4",  GL_FLOATING_PNT, sizeof(GLfloat), 4, 1 },
	{ "uvec2", GL_UNSIGNED_INT, sizeof(GLuint),  2, 1 }, { "uvec3", GL_UNSIGNED_INT, sizeof(GLuint),  3, 1 }, { "uvec4", GL_UNSIGNED_INT, sizeof(GLuint),  4, 1 },

	{ "mat2x2", GL_FLOAT, sizeof(GLfloat), 2, 2 }, { "mat2x3", GL_FLOAT, sizeof(GLfloat), 3, 2 }, { "mat2x4", GL_FLOAT, sizeof(GLfloat), 4, 2 },
	{ "mat3x2", GL_FLOAT, sizeof(GLfloat), 2, 3 }, { "mat3x3", GL_FLOAT, sizeof(GLfloat), 3, 3 }, { "mat3x4", GL_FLOAT, sizeof(GLfloat), 4, 3 },
	{ "mat4x2", GL_FLOAT, sizeof(GLfloat), 2, 4 }, { "mat4x3", GL_FLOAT, sizeof(GLfloat), 3, 4 }, { "mat4x4", GL_FLOAT, sizeof(GLfloat), 4, 4 },
};

struct vxstruct_sd_shaders_list vxsd_shaders;

enum
{
	vxen_sdtype_program,
	vxen_sdtype_vertex,
	vxen_sdtype_fragment
};

/* Check for shader or program link errors. */
static int vxsd_shader_error_check(const char *base_name, GLuint id, int shader_type)
{
	const int is_program = shader_type == vxen_sdtype_program;
	void (APIENTRY *get_iv_func_ptr)(GLuint, GLenum, GLint *) = is_program ? gl.GetProgramiv : gl.GetShaderiv;
	void (APIENTRY *get_ilog_func_ptr)(GLuint, GLint, GLsizei *, GLchar *) = is_program ? gl.GetProgramInfoLog : gl.GetShaderInfoLog;

	GLint result;
	get_iv_func_ptr(id, is_program ? GL_LINK_STATUS : GL_COMPILE_STATUS, &result);
	if (result == 1) return 1;
	
	get_iv_func_ptr(id, GL_INFO_LOG_LENGTH, &result);
	GLchar *info_log = VX_CAST(GLchar *, malloc(VX_CAST(size_t, result)));
	if (info_log) get_ilog_func_ptr(id, result - 1, VX_NULL, info_log);

	const char *shader_type_name = "Unknown";
	if (is_program) shader_type_name = "program";
	else if (shader_type == vxen_sdtype_fragment) shader_type_name = "fragment";
	else if (shader_type == vxen_sdtype_vertex) shader_type_name = "vertex";

	VX_SHADER_WARN("'%s' %s (ID %u) error:\n%s", base_name, shader_type_name, id, info_log);
	return 0;
}

static int vxsd_init_main(
	struct vxstruct_sd_shader_program *program, GLuint *id, GLenum type,
	const char **other_sources, GLsizei sources_count,
	const char *glsl
) {
	other_sources[sources_count - 1] = glsl;
	*id = gl.CreateShader(type);
	gl.ShaderSource(*id, sources_count, other_sources, VX_NULL);
	gl.CompileShader(*id);

	return vxsd_shader_error_check(
		program->name, *id,
		type == GL_VERTEX_SHADER ? vxen_sdtype_vertex : vxen_sdtype_fragment
	) == 0;
}

static int vxsd_sort_attribs(const void *a, const void *b)
{
	const struct vxstruct_sdr_prog_attribs *attrib_a = VX_CAST(const struct vxstruct_sdr_prog_attribs *, a);
	const struct vxstruct_sdr_prog_attribs *attrib_b = VX_CAST(const struct vxstruct_sdr_prog_attribs *, b);
	return VX_CAST(int, attrib_a->loc_index - attrib_b->loc_index);
}

static void vxsd_determine_attributes(struct vxstruct_sd_shader_program *sdr_prog, const char *parser)
{
	#define VX_PARSE_SKIP_WSPC while (*parser && isspace(*parser)) ++parser;

	size_t base_total_stride = 0u, instanced_total_stride = 0u;
	for (; *parser;) {
		/* Looking for: "...= X) in type..." where X is the attribute location and 'type' is the data type. */
		if (*parser++ != '=') continue;
		VX_PARSE_SKIP_WSPC

		int attrib_location = -1;
		int is_instanced = 0; /* Instanced if the number has a '0' at the start. */
		int numbers = 0;

		while (1) {
			const char supposed_number = *parser++;
			if (supposed_number < '0' || supposed_number > '9') break;
			if (attrib_location == -1 && supposed_number == '0') is_instanced = 1;
			attrib_location = ((attrib_location == -1 ? 0 : attrib_location) * 10) + (supposed_number - '0');
			numbers++;
		}
		if (attrib_location == -1) continue;
		if (numbers == 1) is_instanced = 0;
		VX_PARSE_SKIP_WSPC

		/* Specified to be an instanced attribute with a '\n' after the attribute index. */
		if (*parser++ != 'i' || *parser++ != 'n') continue;
		VX_PARSE_SKIP_WSPC

		/* Determine length of the type string. */
		const size_t type_length = VX_CAST(size_t, vxfmt_const_strchrnul(parser, ' ') - parser);

		/* Add an attribute entry from the type name. */
		for (unsigned int i = 0u, last = sizeof vxsd_attrib_types / sizeof *vxsd_attrib_types; i < last; ++i) {
			const vxgl_attrib_pointer_data *attrib_type = vxsd_attrib_types + i;
			if (strncmp(parser, attrib_type->name, type_length) != 0) continue;

			/* Matrix types take up multiple attributes. */
			for (GLint v = 0; v < attrib_type->vectors; ++v) {
				/* Allocate space and add found attribute. */
				void *rlc_attribs = realloc(sdr_prog->attributes, ++sdr_prog->attribs_count * sizeof *sdr_prog->attributes);
				if (!rlc_attribs) VX_ABORT_ALLOCATION();
				else if (sdr_prog->attributes != rlc_attribs) sdr_prog->attributes = VX_CAST(struct vxstruct_sdr_prog_attribs *, rlc_attribs);
	
				struct vxstruct_sdr_prog_attribs *attrib = sdr_prog->attributes + (sdr_prog->attribs_count - 1u);
				attrib->integral = attrib_type->gl_type != GL_FLOAT && attrib_type->gl_type != GL_DOUBLE;
				attrib->loc_index = VX_CAST(unsigned int, attrib_location + v);
				attrib->ogl_type = attrib_type->gl_type;
				attrib->count = attrib_type->count;
				attrib->instanced = is_instanced;
				attrib->list_index = i;
	
				/* Accumulate total stride. */
				const size_t attrib_bytes = VX_CAST(size_t, attrib->count * attrib_type->bytes_each);
				if (is_instanced) instanced_total_stride += attrib_bytes;
				else base_total_stride += attrib_bytes;
			}

			goto next_attribute_search;
		}

		vxlog_free(VX_LOG_WARNING_BIT, vxfmt_text("Sdr %s: Unknown attrib type %.*s", type_length, parser));
	next_attribute_search:
		continue;
	}

	sdr_prog->base_stride = base_total_stride;
	sdr_prog->inst_stride = instanced_total_stride;

	/* Sort attributes and calculate stride. */
	qsort(sdr_prog->attributes, sdr_prog->attribs_count, sizeof(struct vxstruct_sdr_prog_attribs), vxsd_sort_attribs);
	int base_ordered_stride = 0, instanced_ordered_stride = 0;

	for (size_t i = 0u; i < sdr_prog->attribs_count; ++i) {
		struct vxstruct_sdr_prog_attribs *attrib = sdr_prog->attributes + i;
		const int attrib_bytes = attrib->count * vxsd_attrib_types[attrib->list_index].bytes_each;

		if (attrib->instanced) {
			attrib->stride = instanced_ordered_stride;
			instanced_ordered_stride += attrib_bytes;
		} else {
			attrib->stride = base_ordered_stride;
			base_ordered_stride += attrib_bytes;
		}

		VX_SHADER_LOG(
			3, " +Attr LOC:%u INST:%d STR:%d T:%s",
			attrib->loc_index, attrib->instanced, attrib->stride, vxsd_attrib_types[attrib->list_index].name
		);
	}
}

void vxsd_init_shader(
	struct vxstruct_sd_shader_program *sdr_prog,
	char *path, const char *name,
	const char **other_sources, int other_sources_length
) {
	sdr_prog->name = name;
	sdr_prog->pid = gl.CreateProgram();

	char *glsl = vxfile_read(path, VX_NULL);
	if (!glsl) VX_ABORT(vxfmt_text("Could not load shader %s", path));
	VX_FREE(path);

	/* Separate vertex and fragment code. */
	char *end = strchr(glsl, '@');
	if (!end) VX_SHADER_ABORT("Shader %s has no '@' separator", path);
	*end = '\0';

	int any_error = vxsd_init_main(
		sdr_prog, &sdr_prog->vertex, GL_VERTEX_SHADER,
		other_sources, other_sources_length,
		glsl
	);
	any_error |= vxsd_init_main(
		sdr_prog, &sdr_prog->fragment, GL_FRAGMENT_SHADER,
		other_sources, other_sources_length,
		end + 1
	);

	VX_SHADER_LOG(2, "+Sdr %s PROG:%u", name, sdr_prog->pid);

	vxsd_determine_attributes(sdr_prog, glsl);
	VX_FREE(glsl);

	/* Attach compiled vertex and fragment code. */
	gl.AttachShader(sdr_prog->pid, sdr_prog->vertex);
	gl.AttachShader(sdr_prog->pid, sdr_prog->fragment);
	gl.LinkProgram(sdr_prog->pid);

	if (!any_error) vxsd_shader_error_check(name, sdr_prog->pid, vxen_sdtype_program);
	
	/* No need for vertex+fragment shaders, using program from now on. */
	gl.DeleteShader(sdr_prog->vertex);
	gl.DetachShader(sdr_prog->pid, sdr_prog->vertex);
	gl.DeleteShader(sdr_prog->fragment);
	gl.DetachShader(sdr_prog->pid, sdr_prog->fragment);

	gl.UseProgram(sdr_prog->pid);

	/* Set texture units and UBO binding. */
	const vxtexture_info *textures = VX_REINT_CAST(const vxtexture_info *, &vxtex_textures);

	for (size_t i = 0u; i < sizeof vxtex_textures / sizeof(vxtexture_info); ++i) {
		const GLint location = gl.GetUniformLocation(sdr_prog->pid, textures[i].tex_name);
		if (location == -1) continue;
		gl.Uniform1iv(location, 1, &textures[i].internal_index);
	}
	for (size_t i = 0u; i < (sizeof vxubo_bases / sizeof *vxubo_bases); ++i) {
		const GLuint index = gl.GetUniformBlockIndex(sdr_prog->pid, vxubo_bases[i]->ubo_name);
		gl.UniformBlockBinding(sdr_prog->pid, index, vxubo_bases[i]->id);
	}
}


void vxsd_destroy_shader(struct vxstruct_sd_shader_program *sdr_prog)
{
	if (!sdr_prog->pid) return;
	gl.DeleteShader(sdr_prog->pid);
	VX_FREE(sdr_prog->attributes);
	sdr_prog->pid = 0;
}


int vxsd_get_location(const struct vxstruct_sd_shader_program *sdr_prog, const char *name)
{
	const int loc = gl.GetUniformLocation(sdr_prog->pid, name);
	if (loc == -1) VX_SHADER_WARN("Sdr %s - unknown uniform %s", sdr_prog->name, name);
	return loc;
}

void vxsd_set_ints(const struct vxstruct_sd_shader_program *sdr_prog, int location, const int *values, int count)
{
	if (sdr_prog) gl.UseProgram(sdr_prog->pid);
	gl.Uniform1iv(location, count, values);
}
void vxsd_set_floats(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *values, int count)
{
	if (sdr_prog) gl.UseProgram(sdr_prog->pid);
	gl.Uniform1fv(location, count, values);
}

void vxsd_set_vec2(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *float_vals)
{
	if (sdr_prog) gl.UseProgram(sdr_prog->pid);
	gl.Uniform2f(location, float_vals[0], float_vals[1]);
}
void vxsd_set_vec3(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *float_vals)
{
	if (sdr_prog) gl.UseProgram(sdr_prog->pid);
	gl.Uniform3f(location, float_vals[0], float_vals[1], float_vals[2]);
}
void vxsd_set_vec4(const struct vxstruct_sd_shader_program *sdr_prog, int location, const float *float_vals)
{
	if (sdr_prog) gl.UseProgram(sdr_prog->pid);
	gl.Uniform4f(location, float_vals[0], float_vals[1], float_vals[2], float_vals[3]);
}
