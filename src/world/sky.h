#pragma once
#ifndef SOURCE_WORLD_SKY_VXL_HDR
#define SOURCE_WORLD_SKY_VXL_HDR
/* Skybox elements handler. */

#include "directives/dextern.h"

#define VX_SKY_DAY_SECONDS (1200.0)

#define VX_SKY_STARS_COUNT (100000)
#define VX_SKY_STARS_CYCLE_THRESHOLD (0.4f)

#define VX_SKY_CLOUDS_COUNT (1200)
#define VX_SKY_CLOUDS_BASE_Y (102.7f)
#define VX_SKY_CLOUDS_MAX_DISTANCE (4000.0f)

/* Initializes all the sky elements. */
VX_C_FUNC int vxsky_init(VX_NO_ARG);

#endif
