#pragma once
#ifndef SOURCE_DIRECTIVES_FREE_VXL_HDR
#define SOURCE_DIRECTIVES_FREE_VXL_HDR
/* Deallocation directive. */

#include "directives/dword.h"

/* Deallocate pointer and set it to a null pointer for easier debugging. */
#define VX_FREE(ptr) do { if ((ptr) != VX_NULL) { free(ptr); ptr = VX_NULL; } } while (0)
/* Same as normal free macro, but do not set the pointer to null. */
#define VX_FREE_NOSET(ptr) do { if ((ptr) != VX_NULL) { free(ptr); } } while (0)

#endif
