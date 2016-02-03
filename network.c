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

#include <string.h>

#include <dlfcn.h>

#include "lionfs.h"
#include "modules/common.h"

struct nmodule
{
	const char *name;
	const char *path;
};

static struct nmodule modules[] =
{
	{"http", "modules/http"},
	{NULL, NULL},
};

static size_t
(*func_get_data)(void*, char*, long long, size_t);

static long long
(*func_get_length)(char*);

static int
(*func_get_valid)(char*);

static int
(*func_get_info)(_info_t *, char*);

static int
open_syms(void *handle)
{
	if((func_get_data = dlsym(handle, "get_data")) == NULL)
		return -1;

	if((func_get_length = dlsym(handle, "get_length")) == NULL)
		goto _go_close0;

	if((func_get_valid = dlsym(handle, "get_valid")) == NULL)
		goto _go_close1;

	if((func_get_info = dlsym(handle, "get_info")) == NULL)
		goto _go_close2;

	return 0;

_go_close2:
	dlclose(func_get_valid);
_go_close1:
	dlclose(func_get_length);
_go_close0:
	dlclose(func_get_data);

	return -1;
}

static void
close_syms(void)
{
	dlclose(func_get_data);
	dlclose(func_get_length);
	dlclose(func_get_valid);
	dlclose(func_get_info);
	return;
}

size_t
network_file_get_data(char *url, size_t size, long long off, void *data)
{
	return func_get_data(data, url, off, size);
}

long long
network_file_get_length(char *url)
{
	return func_get_length(url);
}

int
network_file_get_valid(char *url)
{
	return func_get_valid(url);
}

int
network_file_get_info(char *url, _file_t *file)
{
	_info_t info;

	if(func_get_info(&info, url) == -1)
		return -1;

	file->size = info.size;
	file->mtime = info.mtime;

	return 0;
}

int
network_open_module(const char *name)
{
	int i;
	void *handle;

	for(i = 0; modules[i].name; i++)
	{
		if(strcmp(modules[i].name, name) != 0)
			continue;

		if((handle = dlopen(modules[i].path, RTLD_NOW)) ==
	NULL)
			break;

		if(open_syms(handle) == -1)
			break;

		return 0;
	}

	return -1;
}

void
network_close_module(void)
{
	return close_syms();
}
