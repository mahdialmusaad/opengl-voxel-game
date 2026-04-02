#pragma once
#ifndef SOURCE_DIRECTIVES_SETTINGS_VXL_HDR
#define SOURCE_DIRECTIVES_SETTINGS_VXL_HDR
/* Debugging settings. */

#include "io/format.h" /* IWYU pragma: export */
#include "io/logs.h" /* IWYU pragma: export */

#define VX_LOG_SETS_VERBOSITY(verbosity) (VX_LOG_VERBOSE##verbosity##_REQ)

#define  VX_CONTEXT_LOG(verbosity, msg, ...) vxlog_free(VX_LOG_SETS_VERBOSITY(verbosity) | VX_LOG_CONTEXT_BIT, vxfmt_text(msg, __VA_ARGS__))
#define  VX_WRLDMGR_LOG(verbosity, msg, ...) vxlog_free(VX_LOG_SETS_VERBOSITY(verbosity) | VX_LOG_WRLDMGR_BIT, vxfmt_text(msg, __VA_ARGS__))
#define  VX_TEXTMGR_LOG(verbosity, msg, ...) vxlog_free(VX_LOG_SETS_VERBOSITY(verbosity) | VX_LOG_TEXTMGR_BIT, vxfmt_text(msg, __VA_ARGS__))
#define  VX_ELEMENT_LOG(verbosity, msg, ...) vxlog_free(VX_LOG_SETS_VERBOSITY(verbosity) | VX_LOG_ELEMENT_BIT, vxfmt_text(msg, __VA_ARGS__))
#define   VX_SHADER_LOG(verbosity, msg, ...) vxlog_free(VX_LOG_SETS_VERBOSITY(verbosity) | VX_LOG_SHADERS_BIT, vxfmt_text(msg, __VA_ARGS__))

#endif
