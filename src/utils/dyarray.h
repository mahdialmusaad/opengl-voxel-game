#pragma once
#ifndef SOURCE_UTILS_DYARRAY_VXL_HDR
#define SOURCE_UTILS_DYARRAY_VXL_HDR
/* Dynamic array declaration. */

#include "directives/dextern.h"
#include "directives/dword.h"

#include <stddef.h>

/* Dynamic array object. */
typedef struct
{
	void *data;
	size_t size, capacity;
	size_t element_size : (sizeof(size_t) * 8) - 1, reserve_double : 1;
} vxdy_array;

/* Initialization for a dynamic array. */
#define VX_DYARRAY_INIT(elem_size, do_double) { VX_NULL, 0u, 0u, elem_size, do_double }

VX_C_START

/* Reserve the given amount of elements in the given dynamic array.
   Returns whether allocation failed or not. */
int vxdy_array_reserve(vxdy_array *array, size_t elems_to_reserve);
/* Add an item to the end of the dynamic array by copying.
   A null item will only allocate data.
   Returns a pointer to the item/allocated space, or null on failure. */
void *vxdy_array_add(vxdy_array *array, void *item);
/* Same as adding, but for multiple items.
   Null items are not accepted. */
void *vxdy_array_addmult(vxdy_array *array, void *items, size_t count);
/* Remove a number of items starting at the given index. */
void vxdy_array_remove(vxdy_array *array, size_t index, size_t count);
/* Free memory used by the dynamic array. */
void vxdy_array_free(vxdy_array *array);

VX_C_END

#endif
