/*
 * lionfs, The Link Over Network File System
 * Copyright (C) 2016,2017  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
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

#include <pthread.h>
#include <sys/types.h>

#include "linked_list.h"

typedef struct
{
	struct list_head list_entry;
	pthread_rwlock_t lock;
	/*
	 * to modify `path` you need list and file write-locks held
	 */
	char *path;
	char *url;
	long long size;
	mode_t mode;
	time_t mtime; /* Last Modified */
} lionfile_t;
