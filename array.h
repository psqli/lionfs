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

typedef void** Array;

typedef signed int ArrayPosition;

typedef struct _header_t ArrayHeader;

/* The header structure */
struct _header_t
{
	union
	{
		ArrayPosition array_position;
		ArrayPosition next;
	};
	size_t size;
};

#define HEADER_SIZE sizeof(struct _header_t)

/* array specific functions */
Array
array_new(size_t);

int
array_del(Array);

size_t
array_get_size(Array);

/* object specific functions */
void*
array_object_new(Array, size_t);

int
array_object_del(Array, ArrayPosition);

int
array_object_link(Array, void*);

int
array_object_unlink(Array, ArrayPosition);

int
array_object_change(Array, ArrayPosition, ArrayPosition);

ArrayPosition
array_object_get_position(Array, void*);

ArrayPosition
array_object_get_last(Array);

void*
array_object_alloc(size_t);

int
array_object_free(void*);
