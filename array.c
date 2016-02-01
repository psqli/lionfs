/*
 * lionfs, The Link Over Network File System
 * Copyright (C) 2016  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Originally designed to The Opencall Project
 */

#include <stdlib.h>
#include <errno.h>

#include "array.h"

/* Total allocated arrays (for control purposes) */
static unsigned int allocated_arrays = 0;

/**
 * get_header() Return the header of array or object.
 *
 * @p addr The address of array or object.
 */
static inline ArrayHeader*
get_header(void *addr)
{
	return (ArrayHeader*) (addr - HEADER_SIZE);
}

/**
 * get_begin() Return the begining of array or object.
 *
 * @p addr The address of array or object.
 */
static inline void*
get_begin(void *addr)
{
	return (addr + HEADER_SIZE);
}

/**
 * get_size() Return size of an array (in objects supported) or an object
 * (in bytes).
 *
 * @p addr The address of array or object.
 */
static inline size_t
get_size(void *addr)
{
	return (size_t) get_header(addr)->size;
}

/**
 * next_get() Return the next free/NULL space in array.
 *
 * @p array The array.
 */
static inline ArrayPosition
next_get(Array array)
{
	return (ArrayPosition) get_header((void*) array)->next;
}

/**
 * next_add() Increment in one the counter of "next free space".
 *
 * @p array The array.
 */
static inline void
next_add(Array array)
{
	get_header((void*) array)->next++;
}

/**
 * next_sub() Decrement in one the counter of "next free space".
 *
 * @p array The array.
 */
static inline void
next_sub(Array array)
{
	get_header((void*) array)->next--;
}

/**
 * object_get_last() Return the index of last object in array. If there isn't
 * any object it will return -1.
 *
 * @p array The array.
 */
static inline ArrayPosition
object_get_last(Array array)
{
	return (next_get(array) - 1);
}

/**
 * index_get_last() Get the last index number of array.
 *
 * @p array The array.
 */
static inline int
index_get_last(Array array)
{
	return get_size((void*) array) - 1;
}

/**
 * free_space() Free non-NULL space in array and turn it NULL.
 *
 * @p space A non-NULL space in an array.
 */
static void
free_space(void **space)
{
	free(get_header(*space));

	*space = NULL;

	return;
}

/**
 * init_array() This function initializes the array with all members set to
 * NULL.
 *
 * @p array The array.
 */
static void
init_array(Array array)
{
	unsigned int i;

	for(i = 0; i <= index_get_last(array); i++)
		array[i] = NULL;

	return;
}

/**
 * free_array()
 * "Free non-NULL spaces in array"
 *
 * @p array The array.
 */
static void
free_array(Array array)
{
	unsigned int i;

	for(i = 0; i <= object_get_last(array); i++)
	{
		if(array[i] == NULL)
			continue;

		free_space(array + i);
	}

	return;
}

/**
 * get_empty_space() This function search for a NULL space in array and deliver
 * it address (for manipule it later) as return variable.
 *
 * @p array The array.
 * @p array_position When found a NULL space, put it position here.
 */
static void**
get_empty_space(Array array, ArrayPosition *array_position)
{
	*array_position = next_get(array);

	if(*array_position > index_get_last(array))
		return NULL;

	return (array + *array_position); /* &array[X] = (array + X) */
}

/**
 * object_move() Move object in array from `src` to `dest`. The `dest` position
 * need to be NULL"
 *
 * @p array The array.
 * @p src Source position.
 * @p dest Destination position (need to be NULL)"
 */
static void
object_move(Array array, ArrayPosition src, ArrayPosition dest)
{
	array[dest] = array[src];
	array[src] = NULL;

	/* Assume the correct position of `dest` */
	get_header(array[dest])->array_position = dest;

	return;
}

/**
 * object_change() Change two objects position in array.
 *
 * @p array The array.
 * @p array_position_a The position of object A in array.
 * @p array_position_b The position of object B in array.
 */
static void
object_change(Array array, ArrayPosition array_position_a,
	ArrayPosition array_position_b)
{
	void *tmp_ptr;

	/* Cache `array[position_a]` and equal it with `array[position_b]` */
	tmp_ptr = array[array_position_a];
	array[array_position_a] = array[array_position_b];

	/* Now we will equal `array[position_b]` with cache of
	`array[position_a]` */
	array[array_position_b] = tmp_ptr;

	/* Now we need to assume the correct position in each `array` changed */
	get_header(array[array_position_a])->array_position = array_position_a;

	get_header(array[array_position_b])->array_position = array_position_b;

	return;
}

/**
 * align_array() Align spaces of array for doesn't stay with blanck (NULL)
 * spaces between spaces with objects. Return how many moves were did.
 *
 * @p array The array.
 * @p array_position Position of NULL space deleted with `array_object_del()`.
 *
 * Example:
 * If the array was like this:
 *  object, object, NULL, object
 * It's going to be like this:
 *  object, object, object, NULL
 */
static int
align_array(Array array, ArrayPosition array_position)
{
#ifdef ALIGN_LAST_FIRST
	object_move(array, object_get_last(array), array_position);
#else
	unsigned int backward;
	unsigned int total_moves;

	unsigned int i;

	backward = total_moves = 0;

	for(i = array_position; i <= object_get_last(array) ||
	i <= index_get_last(array); i++)
	{
		if(array[i])
		{
			if(backward)
			{
				object_move(array, i, (i - backward));
				total_moves++;
			}
		}
		else
			backward++;
	}

	return total_moves;
#endif
}

static int
object_insert(void *object)
{}

static int
object_remove(ArrayPosition array_position)
{}

/**
 * array_object_new() Allocate space for an object and mark its address in
 * array.
 *
 * @p array The array.
 * @p object_size Size of object to allocate.
 */
void*
array_object_new(Array array, size_t object_size)
{
	ArrayHeader *header;
	void **space;

	ArrayPosition array_position;

	space = get_empty_space(array, &array_position);

	if(space == NULL)
		return NULL;

	header = malloc(HEADER_SIZE + object_size);

	if(header == NULL)
		return NULL;

	header->array_position = array_position;
	header->size = object_size;

	*space = get_begin(header);

	next_add(array);

	return *space;
}

/**
 * array_object_del() Free space allocated with `arrayspace_new()` and mark in
 * array as `NULL`.
 *
 * @p array The array.
 * @p array_position Position of object in array to delete.
 */
int
array_object_del(Array array, ArrayPosition array_position)
{
	if(array[array_position] == NULL)
		return -EINVAL;

	free_space(array + array_position);

	if(object_get_last(array) != array_position)
		align_array(array, array_position);

	next_sub(array);

	return 0;
}

int
array_object_link(Array array, void *object)
{
	if(object == NULL)
		return -EINVAL;

	void **space;

	ArrayPosition array_position;

	space = get_empty_space(array, &array_position);

	if(space == NULL)
		return -ENOMEM;

	get_header(object)->array_position = array_position;

	*space = object;

	next_add(array);

	return 0;
}

int
array_object_unlink(Array array, ArrayPosition array_position)
{
	if(array[array_position] == NULL)
		return -EINVAL;

	array[array_position] = NULL;

	if(object_get_last(array) != array_position)
		align_array(array, array_position);

	next_sub(array);

	return 0;
}

/**
 * array_object_change() Change two objects in array. It uses `object_change()`
 * or `object_move()` to do the movement.
 *
 * @p array The array.
 * @p array_position_a Position A.
 * @p array_position_b Position B.
 */
int
array_object_change(Array array, ArrayPosition array_position_a,
	ArrayPosition array_position_b)
{
	if(!(array[array_position_a] && array[array_position_b]))
		return -EINVAL;

	object_change(array, array_position_a, array_position_b);

	return 0;
}

/**
 * array_object_get_position() Get the position of an object.
 *
 * @p array The array.
 */
ArrayPosition
array_object_get_position(Array array, void *object)
{
	unsigned int i;

	for(i = 0; i < get_size(array); i++)
	{
		if(object == array[i])
			return i;
	}

	return -1;
}

/**
 * array_object_get_last() Get the position of last object. Return -1 if there
 * is not any.
 *
 * @p array The array.
 */
ArrayPosition
array_object_get_last(Array array)
{
	return object_get_last(array);
}

/**
 * array_object_alloc() Alloc object for insert in array with link/unlink
 * system. It alloc size plus HEADER_SIZE.
 *
 * @p size The size of object.
 */
void*
array_object_alloc(size_t size)
{
	void *ret;

	if(size == 0)
		return NULL;

	if((ret = malloc(size + HEADER_SIZE)) == NULL)
		return NULL;

	((ArrayHeader*) ret)->array_position = 0;
	((ArrayHeader*) ret)->size = size;

	return get_begin(ret);
}

/**
 * array_object_free() Free an object allocated with `array_object_alloc()`.
 * It frees the pointer minus HEADER_SIZE.
 *
 * @p object Pointer to object.
 */
int
array_object_free(void *object)
{
	if(object == NULL)
		return -EINVAL;

	free(get_header(object));

	return 0;
}

/*
 * Array specific functions
 */

/**
 * array_new() Allocate new array with `size`.
 *
 * @p size Size (in members).
 */
Array
array_new(size_t size)
{
	if(size == 0)
		return NULL;

	ArrayHeader *header;
	Array array;

	header = malloc( HEADER_SIZE + ( sizeof(void*) * size ) );

	if(header == NULL)
		return NULL;

	header->next = 0;
	header->size = size;

	array = (Array) get_begin(header);

	init_array(array);

	allocated_arrays++;

	return array;
}

/**
 * array_del() Delete an array allocated with `array_new()`.
 *
 * @p array The array (to delete).
 */
int
array_del(Array array)
{
	if(next_get(array) != 0)
		free_array(array);

	free(get_header((void*) array));

	allocated_arrays--;

	return 0;
}

/**
 * array_get_size() Get size of an array.
 *
 * @p array The array.
 */
size_t
array_get_size(Array array)
{
	return get_size(array);
}
