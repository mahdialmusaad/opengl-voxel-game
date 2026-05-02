#include "io/logs.h"
#include "io/format.h"

#include "directives/dfree.h"
#include "directives/dcast.h"
#include "directives/dos.h"

#include "graphics/glenum.h"
#include "graphics/glfw.h"

#include <stdlib.h>
#include <stdio.h>

struct vxlog_info_obj vxlog_info = {
	VX_NULL, 1, 1, 0, 0
};

#if VX_WINDOWS == 1
#define VX_PRINT_RED
#define VX_PRINT_YELLOW
#define VX_PRINT_SIZE(x) 0
#else
#define VX_PRINT_RED     "\x1b[31m"
#define VX_PRINT_YELLOW  "\x1b[33m"
#define VX_PRINT_SIZE(x) (sizeof(x) - 1)
#endif

/* Write to both the terminal and the log (if it exists). Log text can be offset to avoid adding terminal colour escapes. */
#define VX_LOG(log_offset, text, ...) do {\
	has_col += (log_offset) != 0;\
	if (vxlog_info.terminal_logging && terminal_stream) fprintf(terminal_stream, text __VA_ARGS__);\
	if (vxlog_info.file_logging) fprintf(VX_CAST(FILE *, vxlog_info.vxlog_stream), (text) + (log_offset) __VA_ARGS__);\
} while (0)

static int vxlog_is_problem(unsigned int log_bit_options)
{
	return (log_bit_options & VX_LOG_ERROR_BIT) | (log_bit_options & VX_LOG_WARNING_BIT) | (log_bit_options & VX_LOG_GLFW3_BIT);
}

static int vxlog_bit_do_terminal(unsigned int log_bit_options)
{
	int verbosity_req = 0;

	if (log_bit_options & VX_LOG_VERBOSE3_REQ) verbosity_req = 3;
	else if (log_bit_options & VX_LOG_VERBOSE2_REQ) verbosity_req = 2;
	else if (log_bit_options & VX_LOG_VERBOSE1_REQ) verbosity_req = 1;
	else if (log_bit_options & VX_LOG_VERBOSE0_REQ) verbosity_req = 0;
	else if (log_bit_options & VX_LOG_SECTION_BITS) verbosity_req = 1;

	return vxlog_info.verbosity >= verbosity_req;
}

void vxlog_msg(unsigned int log_bit_options, const char *msg)
{
	const int to_err_stream = vxlog_is_problem(log_bit_options); 
	FILE *terminal_stream = to_err_stream || vxlog_bit_do_terminal(log_bit_options) ? (to_err_stream ? stderr : stdout) : VX_NULL;
	int has_col = 0;

	#define VA_COMMA ,

	if (log_bit_options & VX_LOG_EXTRANL_BIT) VX_LOG(0, "\n",);
	if (!(log_bit_options & VX_LOG_NOTIME_BIT)) VX_LOG(0, "[ %.3fms ] ", VA_COMMA glfwGetTime() * 1000.0);

	if (log_bit_options & VX_LOG_ERROR_BIT)   VX_LOG(VX_PRINT_SIZE(VX_PRINT_RED), VX_PRINT_RED "[  ABORT  ] ",);
	if (log_bit_options & VX_LOG_SHADERS_BIT) VX_LOG(0, "[ SHADERS ] ",);
	if (log_bit_options & VX_LOG_GLFW3_BIT)   VX_LOG(VX_PRINT_SIZE(VX_PRINT_YELLOW), VX_PRINT_YELLOW "[  GLFW3  ] ",);
	if (log_bit_options & VX_LOG_CONTEXT_BIT) VX_LOG(0, "[ CONTEXT ] ",);
	if (log_bit_options & VX_LOG_ELEMENT_BIT) VX_LOG(0, "[ ELEMENT ] ",);
	if (log_bit_options & VX_LOG_TEXTMGR_BIT) VX_LOG(0, "[ TEXTMGR ] ",);
	if (log_bit_options & VX_LOG_WRLDMGR_BIT) VX_LOG(0, "[ WRLDMGR ] ",);
	if (log_bit_options & VX_LOG_WARNING_BIT) VX_LOG(VX_PRINT_SIZE(VX_PRINT_YELLOW), VX_PRINT_YELLOW "[ WARNING ] ",);

	VX_LOG(0, "%s%s", VA_COMMA msg, ((log_bit_options & VX_LOG_NOSEPNL_BIT) ? "" : "\n"));
#if !VX_WINDOWS
	if (terminal_stream && has_col) fprintf(terminal_stream, "\x1b[0m");
#else
	(void)(has_col);
#endif
}

void vxlog_free(unsigned int log_bit_options, char *msg)
{
	vxlog_msg(log_bit_options, msg);
	VX_FREE(msg);
}

void vxlog_glfw_err(int error_code, const char *error_message)
{
	if (error_code == GLFW_FEATURE_UNAVAILABLE) return;
	vxlog_msg(VX_LOG_GLFW3_BIT, vxfmt_text("%d - %s", error_code, error_message));
}

void VX_APIENTRY vxlog_ogl_debugout(
	GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length,
	const GLchar *msg, const void *userparam
) {
	const char *source_str = VX_NULL, *type_str = VX_NULL, *severity_str = VX_NULL;
	
	(void)(userparam);
	(void)(length);
	
	if (id == 131185) return;

	switch (source) {
	case GL_DEBUG_SOURCE_API: source_str = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source_str = "Window"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "Shader"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: source_str = "External"; break;
	case GL_DEBUG_SOURCE_APPLICATION: source_str = "App"; break;
	default: source_str = "Other"; break;
	}
	switch (type) {
	case GL_DEBUG_TYPE_ERROR: type_str = "Error"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_str = "Undefined"; break; 
	case GL_DEBUG_TYPE_PORTABILITY: type_str = "Portability"; break; 
	case GL_DEBUG_TYPE_PERFORMANCE: type_str = "Performance"; break; 
	case GL_DEBUG_TYPE_MARKER: type_str = "Marker"; break; 
	default: type_str = "Other"; break; 
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH: severity_str = "High"; break;
	case GL_DEBUG_SEVERITY_MEDIUM: severity_str = "Medium"; break;
	case GL_DEBUG_SEVERITY_LOW: severity_str = "Low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "Notification"; break;
	default: severity_str = "Other"; break;
	}

	vxlog_free(VX_LOG_WARNING_BIT, vxfmt_text("[ %s ][ %s ][ %s ][ %u ] %s", severity_str, type_str, source_str, id, msg));
}

void vxlog_abort_impl(const char *msg, const char *file, int line)
{
	vxlog_msg(VX_LOG_ERROR_BIT, *msg == '\1' ? "Failed to allocate memory" : vxfmt_text("%s:%d - %s", file, line, msg));
	abort();
}
