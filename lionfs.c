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

/*
 * TODO It's not working!
 * Create nodeids for fakefiles after symlink have been created.
 * nodeid 1 is "/"
 * nodeids 2 v 4 v 6 v 8 ... are symlinks
 * nodeids   3 ^ 5 ^ 7 ^ 9 ... are fakefiles
 */

/* TODO
 * # No nodeids for fakefiles.
 */

_filelist_t filelist = { 0, };

inline time_t
get_mtime(int fid)
{
	return (filelist.file[fid])->mtime;
}

inline mode_t
get_mode(int fid)
{
	return (filelist.file[fid])->mode;
}

inline long long
get_size(int fid)
{
	return (filelist.file[fid])->size;
}

inline char*
get_url(int fid)
{
	return (filelist.file[fid])->url;
}

inline char*
get_path(int fid)
{
	return (filelist.file[fid])->path;
}

int
get_fid_by_path(const char *path)
{
	int i;

	for(i = 0; i < filelist.count; i++)
	{
		if(strcmp(path, get_path(i)) == 0)
			return i;
	}
	return -ENOENT;
}

int
get_fid_by_ff(const char *path)
{
	if(strncmp(path, "/.ff/", 5) == 0)
	{
		path += 4;
		return get_fid_by_path(path);
	}

	return -1;
}

int
lion_getattr(const char *path, struct stat *buf)
{
	int fid;

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
		buf->st_nlink = 1;
		return 0;
	}

	if((fid = get_fid_by_ff(path)) >= 0)
	{
		buf->st_mode = S_IFREG | 0444;
		buf->st_size = get_size(fid);
		buf->st_mtime = get_mtime(fid);
		buf->st_nlink = 0;
		return 0;
	}

	if((fid = get_fid_by_path(path)) < 0)
		return -ENOENT;

	buf->st_mode = get_mode(fid);
	buf->st_size = get_size(fid);
	buf->st_mtime = get_mtime(fid);
	buf->st_nlink = 1;

	return 0;
}

int
lion_readlink(const char *path, char *buf, size_t len)
{
	int fid;

	if((fid = get_fid_by_path(path)) < 0)
		return -ENOENT;

	sprintf(buf, ".ff/%s", get_path(fid) + 1);

	return 0;
}

int
lion_unlink(const char *path)
{
	int fid;
	_file_t *file;

	if((fid = get_fid_by_path(path)) < 0)
		return -ENOENT;

	file = filelist.file[fid];

	array_object_unlink((Array) filelist.file, fid);

	filelist.count--;

	free(file->path);
	free(file->url);

	array_object_free(file);

	return 0;
}

int
lion_symlink(const char *url, const char *path)
{
	_file_t *file;

	if(filelist.count == MAX_FILES)
		return -ENOMEM;

	if(get_fid_by_path(path) >= 0)
		return -EEXIST;

	if(network_file_get_valid((char*) url))
		return -EHOSTUNREACH;

	file = array_object_alloc(sizeof(_file_t));

	if(file == NULL)
		return -ENOMEM;

	file->path = malloc(strlen(path) + 1);
	file->url = malloc(strlen(url) + 1);

	strcpy(file->path, path);
	strcpy(file->url, url);

	file->mode = S_IFLNK | 0444;

	network_file_get_info(file->url, file);

	array_object_link((Array) filelist.file, file);

	filelist.count++;

	return 0;
}

int
lion_rename(const char *oldpath, const char *newpath)
{
	int fid;
	char **path_pointer;
	size_t newsize;

	if((fid = get_fid_by_path(oldpath)) < 0)
		return -ENOENT;

	if(get_fid_by_path(newpath) >= 0)
		return -EEXIST;

	path_pointer = &(filelist.file[fid])->path;
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
	int fid;

	if((fid = get_fid_by_ff(path)) < 0)
		return -ENOENT;

	if(off < get_size(fid))
	{
		if(off + size > get_size(fid))
			size = get_size(fid) - off;
	}
	else
		return 0;

	return network_file_get_data(get_url(fid), size, off, buf);
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

	for(i = 0; i < filelist.count; i++)
		filler(buf, get_path(i) + 1, NULL, 0);

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

	if((filelist.file = (_file_t**) array_new(MAX_FILES)) == NULL)
		return 1;

	if(network_open_module("http") == -1)
	{
		ret = 1;
		goto _go_free;
	}

	fuse_main(argc, argv, &fuseopr, NULL);

	// network_close_module(); TODO do not working!

_go_free:
	array_del((Array) filelist.file);

	return ret;
}
