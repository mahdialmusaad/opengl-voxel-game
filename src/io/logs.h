#pragma once
#ifndef SOURCE_IO_LOGS_VXL_HDR
#define SOURCE_IO_LOGS_VXL_HDR
/* Logging and error handling functions. */

#include "directives/dextern.h"

#include "graphics/gltypes.h"

#define VX_LOG_VERBOSE0_REQ (16384u)
#define VX_LOG_VERBOSE3_REQ (8192u)
#define VX_LOG_VERBOSE2_REQ (4096u)
#define VX_LOG_VERBOSE1_REQ (2048u)

#define VX_LOG_SECTION_BITS (8u | 16u | 32u | 64u | 128u)

#define VX_LOG_ERROR_BIT   (1024u)
#define VX_LOG_WARNING_BIT (512u)
#define VX_LOG_GLFW3_BIT   (256u)
#define VX_LOG_SHADERS_BIT (128u)
#define VX_LOG_CONTEXT_BIT (64u)
#define VX_LOG_ELEMENT_BIT (32u)
#define VX_LOG_TEXTMGR_BIT (16u)
#define VX_LOG_WRLDMGR_BIT (8u)
#define VX_LOG_NOTIME_BIT  (4u)
#define VX_LOG_EXTRANL_BIT (2u)
#define VX_LOG_NOSEPNL_BIT (1u)
#define VX_LOG_DEFAULT_BIT (0u)

/* Global logging information. */
struct vxlog_info_obj {
	void *vxlog_stream;
	int terminal_logging, file_logging;
	int verbosity, padding;
};
extern struct vxlog_info_obj vxlog_info;


VX_C_START

/* Log a message to stdout with a relative timestamp.
   Options are available in the form of a bitfield (see VX_LOG macros). */
void vxlog_msg(unsigned int log_bit_options, const char *msg);
/* Same as vxlog_log, but frees the text afterwards. */
void vxlog_free(unsigned int log_bit_options, char *msg);

/* Warn of an error from GLFW. */
void vxlog_glfw_err(int error_code, const char *error_message);

/* Log OpenGL message. */
void VX_APIENTRY vxlog_ogl_debugout(
	GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length,
	const GLchar *msg, const void *userparam
);

/* Internal handler for aborting. */
#if defined(_MSC_VER)
    __declspec(noreturn)
#else
    __attribute__((noreturn))
#endif
    void vxlog_abort_impl(const char *msg, const char *file, int line);

VX_C_END

/* Abort with the given error message. */
#define VX_ABORT(error_msg) vxlog_abort_impl(error_msg, VX_FILE_ID, __LINE__)
/* Abort due to a memory allocation failure. */
#define VX_ABORT_ALLOCATION() VX_ABORT("\1")

#endif
