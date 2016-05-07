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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>

#include "lionfs.h"
#include "network.h"
#include "array.h"

#define file_count (array_object_get_last((Array) filelist.file) + 1)

_filelist_t filelist;

lionfile_t*
get_file_by_path(const char *path)
{
	int i;

	for(i = 0; i < file_count; i++)
	{
		if(strcmp(path, filelist.file[i]->path) == 0)
			return filelist.file[i];
	}

	return NULL;
}

lionfile_t*
get_file_by_ff(const char *path)
{
	if(strncmp(path, "/.ff/", 5) == 0)
	{
		path += 4;
		return get_file_by_path(path);
	}

	return NULL;
}

int
lion_getattr(const char *path, struct stat *buf)
{
	lionfile_t *file;

	memset(buf, 0, sizeof(struct stat));

	if(strcmp(path, "/") == 0)
	{
		buf->st_mode = S_IFDIR | 0775;
		buf->st_nlink = 2;
		return 0;
	}

	if(strcmp(path, "/.ff") == 0)
	{
		buf->st_mode = S_IFDIR | 0444;
		buf->st_nlink = 0;
		return 0;
	}

	if((file = get_file_by_ff(path)) != NULL)
	{
		buf->st_mode = S_IFREG | 0444;
		buf->st_size = file->size;
		buf->st_mtime = file->mtime;
		buf->st_nlink = 0;
		return 0;
	}

	if((file = get_file_by_path(path)) == NULL)
		return -ENOENT;

	buf->st_mode = file->mode;
	buf->st_size = file->size;
	buf->st_mtime = file->mtime;
	buf->st_nlink = 1;

	return 0;
}

int
lion_readlink(const char *path, char *buf, size_t len)
{
	lionfile_t *file;

	if((file = get_file_by_path(path)) == NULL)
		return -ENOENT;

	snprintf(buf, len, ".ff/%s", file->path + 1);

	return 0;
}

int
lion_unlink(const char *path)
{
	lionfile_t *file;

	if((file = get_file_by_path(path)) == NULL)
		return -ENOENT;

	array_object_unlink((Array) filelist.file, file);

	free(file->path);
	free(file->url);

	array_object_free(file);

	return 0;
}

int
lion_symlink(const char *url, const char *path)
{
	lionfile_t *file;

	if(file_count == MAX_FILES)
		return -ENOMEM;

	if(get_file_by_path(path) != NULL)
		return -EEXIST;

	if(network_file_get_valid((char*) url))
		return -EHOSTUNREACH;

	file = array_object_alloc(sizeof(lionfile_t));

	if(file == NULL)
		return -ENOMEM;

	file->path = malloc(strlen(path) + 1);
	file->url = malloc(strlen(url) + 1);

	strcpy(file->path, path);
	strcpy(file->url, url);

	file->mode = S_IFLNK | 0444;

	network_file_get_info(file->url, file);

	array_object_link((Array) filelist.file, file);

	return 0;
}

int
lion_rename(const char *oldpath, const char *newpath)
{
	lionfile_t *file;
	char **path_pointer;
	size_t newsize;

	if((file = get_file_by_path(oldpath)) == NULL)
		return -ENOENT;

	if(get_file_by_path(newpath) != NULL)
		return -EEXIST;

	path_pointer = &file->path;
	newsize = strlen(newpath) + 1;

	if(newsize == 1)
		return -EINVAL;

	*path_pointer = realloc(*path_pointer, newsize);

	memcpy(*path_pointer, newpath, newsize);

	return 0;
}

int
lion_read(const char *path, char *buf, size_t size, off_t off,
	struct fuse_file_info *fi)
{
	lionfile_t *file;

	if((file = get_file_by_ff(path)) == NULL)
		return -ENOENT;

	if(off < file->size)
	{
		if(off + size > file->size)
			size = file->size - off;
	}
	else
		return 0;

	return network_file_get_data(file->url, size, off, buf);
}

int
lion_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
	struct fuse_file_info *fi)
{
	int i;

	if(strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for(i = 0; i < file_count; i++)
		filler(buf, filelist.file[i]->path + 1, NULL, 0);

	return 0;
}

struct fuse_operations fuseopr =
{
	.getattr = lion_getattr,
	.readlink = lion_readlink,
	.unlink = lion_unlink,
	.symlink = lion_symlink,
	.rename = lion_rename,
	.read = lion_read,
	.readdir = lion_readdir,
};

int
main(int argc, char **argv)
{
	int ret = 0;

	if((filelist.file = (lionfile_t**) array_new(MAX_FILES)) == NULL)
		return 1;

	if(network_open_module("http") == -1)
	{
		ret = 1;
		goto _go_free;
	}

	fuse_main(argc, argv, &fuseopr, NULL);

	network_close_module();

_go_free:
	array_del((Array) filelist.file);

	return ret;
}
