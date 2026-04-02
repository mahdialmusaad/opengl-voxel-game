#pragma once
#ifndef SOURCE_SHADERS_LOADER_VXL_HDR
#define SOURCE_SHADERS_LOADER_VXL_HDR
/* Shader and related values loader. */

#include "directives/dextern.h"

VX_C_START

/* Load all textures, bind UBOs and initialize all shaders. */
void vxsd_init_all(VX_NO_ARG);
/* Destroy all shaders, UBOs and textures. */
void vxsd_destroy(VX_NO_ARG);

VX_C_END

#endif
