#include "utils/dyarray.h"

#include "directives/dword.h"
#include "directives/dcast.h"
#include "directives/dfree.h"
#include "directives/dfast.h"

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

int vxdy_array_reserve(vxdy_array *array, size_t capacity)
{
	if (!capacity) {
		vxdy_array_free(array);
		return 1;
	}

	/* Reallocate memory for the dynamic array. */
	void *rl_data = realloc(array->data, capacity * array->element_size);
	if (VX_UNLIKELY(!rl_data)) return 0;
	if (rl_data != array->data) array->data = rl_data;

	/* Set the capacity of the dynamic array. */
	array->capacity = capacity;
	if (array->capacity < array->size) array->size = array->capacity;

	return 1;
}

void *vxdy_array_add(vxdy_array *array, void *item)
{
	/* Resize array if there is not enough space, returning if allocation failed. */
	if (array->size >= array->capacity) {
		const size_t target_cap = !array->capacity ? (array->reserve_double + 1u) : array->capacity + (array->reserve_double ? array->capacity : 1u);
		if (VX_UNLIKELY(!vxdy_array_reserve(array, target_cap))) return VX_NULL;
	}

	/* Copy given item to the end of the working dynamic array. In case of null, just return the allocated space. */
	if (item) memcpy(VX_CAST(unsigned char *, array->data) + (array->size * array->element_size), item, array->element_size);
	return VX_CAST(unsigned char *, array->data) + (array->element_size * array->size++);
}

void *vxdy_array_addmult(vxdy_array *array, void *items, size_t count)
{
	if (!count) return VX_CAST(unsigned char *, array->data) + (array->element_size * array->size);

	/* Resize array if there is not enough space, returning if allocation failed. */
	if (array->size + count >= array->capacity) {
		const size_t target_cap = array->size + (count * (array->reserve_double + 1u));
		if (VX_UNLIKELY(!vxdy_array_reserve(array, target_cap))) return VX_NULL;
	}

	memcpy(VX_CAST(unsigned char *, array->data) + (array->size * array->element_size), items, array->element_size * count);

	const size_t prev_size = array->size;
	array->size += count;

	return VX_CAST(unsigned char *, array->data) + (array->element_size * prev_size);
}

void vxdy_array_remove(vxdy_array *array, size_t index, size_t count)
{
	unsigned char *data_ptr = VX_CAST(unsigned char *, array->data);
	const size_t end_bytes = (index + count) * array->element_size;
	memmove(
		data_ptr + (index * array->element_size),
		data_ptr + end_bytes,
		(array->size * array->element_size) - end_bytes
	);
	if (!(array->size -= count)) vxdy_array_free(array);
}

void vxdy_array_free(vxdy_array *array)
{
	VX_FREE(array->data);
	array->data = VX_NULL;
	array->capacity = 0u;
	array->size = 0u;
}
