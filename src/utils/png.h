#pragma once
#ifndef SOURCE_UTILS_PNG_VXL_HDR
#define SOURCE_UTILS_PNG_VXL_HDR
/* PNG handling. */

#include "directives/dextern.h"

#include <stdint.h>

VX_C_START

/* Allocates and store the pixels from the given PNG file path, and saves information to the corresponding pointers. */
int vxpng_load(const char *path, uint8_t **pixels, uint32_t *width, uint32_t *height, int *num_channels);
/* Save the pixels with the given resolution to the given PNG file path. */
int vxpng_save(const char *path, const uint8_t *pixels, uint32_t width, uint32_t height);

VX_C_END

#endif
