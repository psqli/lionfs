#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#define FUSE_USE_VERSION 26

#include <fuse.h>

#undef ALIGN_LAST_FIRST
#define ARRAY_ALIGN
#define ARRAY_LINKUNLINK

#include "lionfs.h"
#include "array.h"

/*
 * nodeid 1 is "/"
 * nodeids 2 v 4 v 6 v 8 ... are symlinks
 * nodeids   3 ^ 5 ^ 7 ^ 9 ... are fakefiles
 */

_filelist_t filelist = { 0, };

inline int8_t*
get_binfo(int fid)
{
	return (filelist.file[fid])->binfo;
}

inline int
set_binfo(int fid, int8_t *binfo)
{
	if(binfo == NULL)
		return -1;
	(filelist.file[fid])->binfo = binfo;	
	return 0;
}

inline mode_t
get_mode(int fid)
{
	return (filelist.file[fid])->mode;
}

inline size_t
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
get_fid_by_url(const char *url)
{
	int i;

	for(i = 0; i < filelist.count; i++)
	{
		if(strcmp(url, get_url(i)) == 0)
			return i;
	}
	return -ENOENT;

}

int
remove_fid(int fid)
{
	_file_t *file;
	
	file = filelist.file[fid];

	array_object_unlink((Array) filelist.file, fid);

	filelist.count--;

	free(file->path);
	free(file->url);

	return 0;
}

int
add_fid(const char *path, const char *url, mode_t mode)
{
	_file_t *file;

	file = malloc(sizeof(_file_t));

	if(file == NULL)
		return -1;

	/* we need allocate first */
	file->path = malloc(strlen(path) + 1);
	file->url = malloc(strlen(url) + 1);

	strcpy(file->path, path);
	strcpy(file->url, url);
	file->mode = mode;
	file->size = 13;

	array_object_link((Array) filelist.file, file, sizeof(_file_t));

	filelist.count++;

	return 0;
}

int
rename_fid(int fid, const char *path)
{
	/* TODO */
}

int
fs_getattr(const char *path, struct stat *buf)
{
	int fid;

	char *endptr;

	memset(buf, 0, sizeof(struct stat));

	if(strcmp(path, "/") == 0)
	{
		buf->st_mode = S_IFDIR | 0775;
		buf->st_nlink = 2;
		return 0;
	}

	fid = get_fid_by_path(path);

	if(fid < 0)
	{
		fid = strtol(path + 1, &endptr, 10);
		if((endptr - (path + 1)) != 2)
			return -ENOENT;
		if(fid < 0 || fid > (filelist.count - 1))
			return -ENOENT;
		if(filelist.file[fid] == NULL)
			return -ENOENT;
		buf->st_mode = S_IFREG | 0444;
		buf->st_size = get_size(fid);
		buf->st_nlink = 0;
		return 0;
	}

	buf->st_mode = get_mode(fid);
	buf->st_size = get_size(fid);
	buf->st_nlink = 1;

	return 0;
}

int
fs_readlink(const char *path, char *buf, size_t len)
{
	int fid;

	char *endptr;
#if 0
	char *url;
	size_t urllen;
#endif
	fid = get_fid_by_path(path);

	if(fid < 0)
	{
		fid = strtol(path + 1, &endptr, 10);
		if((endptr - (path + 1)) != 2)
			return -ENOENT;
		if(fid < 0 || fid > (filelist.count - 1))
			return -ENOENT;
		if(filelist.file[fid] == NULL)
			return -ENOENT;
		sprintf(buf, "%s", get_url(fid));
		return 0;
	}

	sprintf(buf, "%.2d", fid);
#if 0
	url = get_url(fid);
	urllen = strlen(url) + 1;

	if(urllen > len)
		return -EINVAL;

	strcpy(buf, url);
#endif
	return 0;
}

int
fs_unlink(const char *path)
{
	int fid;

	fid = get_fid_by_path(path);

	if(fid < 0)
		return -ENOENT;

	remove_fid(fid);

	return 0;
}

int
fs_symlink(const char *linkname, const char *path)
{
	int fid;

	if(get_fid_by_path(linkname) >= 0)
		return -EEXIST;

	fid = add_fid(path, linkname, S_IFLNK | 0444);

	if(fid < 0)
		return -ENOMEM;

	return 0;
}

int
fs_rename(const char *oldpath, const char *newpath)
{
	int fid;

	fid = get_fid_by_path(oldpath);

	if(fid < 0)
		return -ENOENT;

	rename_fid(fid, newpath);

	return 0;
}

int
fs_read(const char *path, char *buf, size_t size, off_t off,
	struct fuse_file_info *fi)
{
	int fid;

	char *endptr;

	int i;

	fid = strtol(path + 1, &endptr, 10);
	if((endptr - (path + 1)) != 2)
		return -ENOENT;
	if(fid < 0 || fid > (filelist.count - 1))
		return -ENOENT;
	if(filelist.file[fid] == NULL)
		return -ENOENT;

	for(i = 0; i < size; i++)
	{
		buf[i] = 'A';
	}

	return size;
}

int
fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
	struct fuse_file_info *fi)
{
	if(strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	int i;

	for(i = 0; i < filelist.count; i++)
	{
		filler(buf, get_path(i) + 1, NULL, 0);
	}
	return 0;
}

struct fuse_operations fuseopr =
{
	.getattr = fs_getattr,
	.readlink = fs_readlink,
	.unlink = fs_unlink,
	.symlink = fs_symlink,
	.rename = fs_rename,
	.read = fs_read,
	.readdir = fs_readdir,
};

int
main(int argc, char **argv)
{
	if((filelist.file = (_file_t**) array_new(MAX_FILES)) == NULL)
		return 1;	

	fuse_main(argc, argv, &fuseopr, NULL);

	array_del((Array) filelist.file);

	return 0;
}
