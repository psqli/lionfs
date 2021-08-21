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

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "modules/common.h"

struct nmodule {
	const char *scheme;
	const char *filename;
	void *handle;

	size_t
	(*func_get_data)(void*, char*, long long, size_t);

	int
	(*func_get_valid)(char*);

	int
	(*func_get_info)(lionfile_info_t*, char*);
};

static struct nmodule modules[] = {
	{ "http",  "curl", },
	{ "https", "curl", },
	{ NULL, },
};

static inline void
clean_pointers(struct nmodule *nm)
{
	nm->func_get_data  = NULL;
	nm->func_get_valid = NULL;
	nm->func_get_info  = NULL;
	return;
}

static int
load_syms(struct nmodule *nm)
{
	if ((nm->func_get_data = dlsym(nm->handle, "get_data")) == NULL)
		return -1;

	if ((nm->func_get_valid = dlsym(nm->handle, "get_valid")) == NULL)
		return -1;

	if ((nm->func_get_info = dlsym(nm->handle, "get_info")) == NULL)
		return -1;

	return 0;
}

static int
close_module(struct nmodule *nm)
{
	if (!nm->handle)
		return -1;

	if (dlclose(nm->handle))
		return -1;

	clean_pointers(nm);

	return 0;
}

#define PATH_SIZE 4096

static int
open_module(struct nmodule *nm)
{
	assert(nm != NULL);

	char tmp_path[PATH_SIZE];

	if (nm->handle)
		/* module was already opened or user didn't call
		 * network_init() function */
		return -1;

	/* try to open module */
	snprintf(tmp_path, PATH_SIZE, "lionfs/modules/%s", nm->filename);
	if ((nm->handle = dlopen(tmp_path, RTLD_NOW)) == NULL) {
		snprintf(tmp_path, PATH_SIZE, "./modules/%s", nm->filename);
		if ((nm->handle = dlopen(tmp_path, RTLD_NOW)) == NULL)
			return -1;
	}

	if (load_syms(nm) == -1) {
		close_module(nm); /* we've already opened the module */
		return -1;
	}
}

static struct nmodule*
find_module_by_scheme(const char *scheme)
{
	int i;

	for (i = 0; modules[i].scheme; i++)
		if(strcmp(modules[i].scheme, scheme) == 0)
			return &modules[i];

	return NULL;
}

/* Define max URL scheme size.
 * Scheme size is: "http://" = 4 Bytes + '\0' = 5 Bytes) */
#define SCHEME_SIZE 8

static struct nmodule*
find_module_by_url(const char *url)
{
	int i;
	char scheme[SCHEME_SIZE];

	for (i = 0; url[i] && i < SCHEME_SIZE; i++) {
		scheme[i] = url[i];
		if (url[i] == ':') {
			scheme[i] = '\0';
			return find_module_by_scheme(scheme);
		}
	}

	return NULL;
}

size_t
network_file_get_data(char *url, size_t size, long long off, void *data)
{
	struct nmodule *nm;

	if ((nm = find_module_by_url(url)) == NULL)
		return 0;

	if (!nm->func_get_data)
		return 0;

	return nm->func_get_data(data, url, off, size);
}

int
network_file_get_valid(char *url)
{
	struct nmodule *nm;

	if ((nm = find_module_by_url(url)) == NULL)
		return -1;

	if (!nm->func_get_valid)
		return -1;

	return nm->func_get_valid(url);
}

int
network_file_get_info(char *url, lionfile_info_t *file_info)
{
	struct nmodule *nm;

	memset((void*) file_info, 0, sizeof(lionfile_info_t));

	if ((nm = find_module_by_url(url)) == NULL)
		return -1;

	if (!nm->func_get_info)
		return -1;

	if (nm->func_get_info(file_info, url) == -1)
		return -1;

	return 0;
}

int
network_open_module(const char *name)
{
	struct nmodule *nm;

	if ((nm = find_module_by_scheme(name)) == NULL)
		return -1;

	return open_module(nm);
}

void
network_open_all_modules(void)
{
	int i;

	for (i = 0; modules[i].scheme; i++)
		open_module(&modules[i]);

	return;
}

int
network_close_module(const char *name)
{
	struct nmodule *nm;

	if((nm = find_module_by_scheme(name)) == NULL)
		return -1;

	return close_module(nm);
}

void
network_close_all_modules(void)
{
	int i;

	for (i = 0; modules[i].scheme; i++)
		close_module(&modules[i]);

	return;
}

/* Before all, we should call this function! */
void
network_init(void)
{
	int i;

	for (i = 0; modules[i].scheme; i++)
		clean_pointers(&modules[i]);
}
