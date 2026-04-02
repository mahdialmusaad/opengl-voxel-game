#pragma once
#ifndef SOURCE_WORLD_GENERATE_VXL_HDR
#define SOURCE_WORLD_GENERATE_VXL_HDR
/* Chunk decorators and generation. */

#include "directives/dextern.h"

#include "vector/wpos.h"

#include <stddef.h>

VX_C_START

/* Generate and decorate surrounding chunks. */
VX_C_FUNC size_t vxwld_generate_around(VX_NO_ARG);
/* Returns whether a chunk is close enough to be rendered. */
int vxwld_offset_close_enough(const wpos *global_offset);

VX_C_END

#endif
