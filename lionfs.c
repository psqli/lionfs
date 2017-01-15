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

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "lionfs.h"
#include "modules/common.h"
#include "network.h"


struct list_head  files;
pthread_rwlock_t  files_lock;


/*
 * *assume list r/w lock is held
 * during this function file list can't be modified (files won't be added or
 * removed) and path is also safe (will not be modified -- see lion_rename() )
 */
static lionfile_t*
get_file_by_path(const char *path)
{
	lionfile_t *file;

	list_for_each_entry(file, &files, list_entry)
		if (strcmp(path, file->path) == 0)
			return file;

	return NULL;
}


// ================
// fuse operations:
//   lion_getattr()   get attributes (information) from a file
//   lion_readlink()  get target of a symbolic link (or get the fakefile ...)
//   lion_unlink()    removes a file -- only symlinks in lionfs :-)
//   lion_symlink()   creates a symlink
//   lion_rename()    renames a file -- only symlinks as lion_unlink()
//   lion_read()      reads content of a file (reads content of fakefiles)
//   lion_readdir()   get files in a directory (get symlinks in lionfs array)
// ================

static int
lion_getattr(const char *path, struct stat *buf)
{
	lionfile_t *file;
	int is_fakefile = 0;

	memset(buf, 0, sizeof(struct stat));

	/* check if path is our root directory */
	if (strcmp(path, "/") == 0) {
		buf->st_mode = S_IFDIR | 0775;
		buf->st_nlink = 2;
		return 0;
	}

	/* check if path is the fakefiles directory */
	if (strcmp(path, "/.ff") == 0) {
		/* fakefiles directory needs to be read-only */
		buf->st_mode = S_IFDIR | 0444;
		buf->st_nlink = 0;
		return 0;
	}

	/* check if path could be a fakefile */
	if (strncmp(path, "/.ff/", 5) == 0) {
		path += 4;
		is_fakefile = 1;
	}

	/* check if file exists */
	pthread_rwlock_rdlock(&files_lock); /* list read lock */
	if ((file = get_file_by_path(path)) == NULL) {
		pthread_rwlock_unlock(&files_lock);
		return -ENOENT;
	}

	pthread_rwlock_rdlock(&file->lock); /* file read lock */
	pthread_rwlock_unlock(&files_lock);

	if (is_fakefile) {
		buf->st_mode = S_IFREG | 0444;
		buf->st_mtime = file->mtime;
		buf->st_nlink = 0;
		buf->st_size = file->size;

		pthread_rwlock_unlock(&file->lock);
		return 0;
	}

	/* we are a symlink */
	buf->st_mode = file->mode | S_IFLNK; /* S_IFLNK = symlink bitmask */
	buf->st_mtime = file->mtime; /* modification time */
	buf->st_nlink = 1; /*number of hard links (here it's not so important)*/
	buf->st_size = file->size;

	pthread_rwlock_unlock(&file->lock);

	return 0;
}

static int
lion_readlink(const char *path, char *buf, size_t len)
{
	lionfile_t *file;

	/* if symlink does not exist we can't proceed */
	pthread_rwlock_rdlock(&files_lock); /* list read lock */
	if ((file = get_file_by_path(path)) == NULL) {
		pthread_rwlock_unlock(&files_lock);
		return -ENOENT;
	}

	pthread_rwlock_rdlock(&file->lock); /* file read lock */
	pthread_rwlock_unlock(&files_lock);

	snprintf(buf, len, ".ff/%s", file->path + 1);

	pthread_rwlock_unlock(&file->lock);

	return 0;
}

static int
lion_unlink(const char *path)
{
	lionfile_t *file;

	/* if symlink does not exist we can't proceed */
	pthread_rwlock_wrlock(&files_lock); /* list write lock */
	if ((file = get_file_by_path(path)) == NULL) {
		pthread_rwlock_unlock(&files_lock);
		return -ENOENT;
	}

	list_del(&file->list_entry);

	pthread_rwlock_unlock(&files_lock);
	pthread_rwlock_wrlock(&file->lock); /* file write lock */

	free(file->path);
	free(file->url);

	pthread_rwlock_unlock(&file->lock);

	pthread_rwlock_destroy(&file->lock);
	free(file);

	return 0;
}

static int
lion_symlink(const char *url, const char *path)
{
	lionfile_t *file;
	lionfile_info_t file_info;

	/* check if URL exists and get its info */
	if (network_file_get_valid((char*) url))
		return -EHOSTUNREACH;
	if (network_file_get_info((char*) url, &file_info))
		return -EHOSTUNREACH;

	file = malloc(sizeof(lionfile_t));
	pthread_rwlock_init(&file->lock, NULL);

	/* if symlink EXISTS we can't proceed */
	pthread_rwlock_wrlock(&files_lock); /* list write lock */
	if (get_file_by_path(path) != NULL) {
		pthread_rwlock_unlock(&files_lock);
		pthread_rwlock_destroy(&file->lock);
		free(file);
		return -EEXIST;
	}

	list_add(&file->list_entry, &files);

	pthread_rwlock_wrlock(&file->lock); /* file write lock */
	pthread_rwlock_unlock(&files_lock);

	file->path = malloc(strlen(path) + 1);
	file->url = malloc(strlen(url) + 1);

	strcpy(file->path, path);
	strcpy(file->url, url);

	/* symlinks are read-only :-) -- fakefiles copy this */
	file->mode = 0444;

	file->size = file_info.size;
	file->mtime = file_info.mtime;

	pthread_rwlock_unlock(&file->lock);

	return 0;
}

static int
lion_rename(const char *oldpath, const char *newpath)
{
	lionfile_t *file;
	size_t newsize;

	newsize = strlen(newpath) + 1;
	if (newsize == 1)
		return -EINVAL;

	/* if newpath EXISTS or oldpath doesn't exist we can't proceed */
	pthread_rwlock_wrlock(&files_lock); /* list write lock */
	if (get_file_by_path(newpath) != NULL) {
		pthread_rwlock_unlock(&files_lock);
		return -EEXIST;
	} else if ((file = get_file_by_path(oldpath)) == NULL) {
		pthread_rwlock_unlock(&files_lock);
		return -ENOENT;
	}

	/*
	 * note that to change path we need list and file write-locks held --
	 * that's because if one is searching by a file with get_file_by_path()
	 * it wouldn't be possible to make sure path is not being changed during
	 * the search -- with both locks path cannot be changed during a search
	 */

	pthread_rwlock_wrlock(&file->lock); /* file write lock */

	file->path = realloc(file->path, newsize);
	memcpy(file->path, newpath, newsize);

	pthread_rwlock_unlock(&file->lock);
	pthread_rwlock_unlock(&files_lock);

	return 0;
}

static int
lion_read(const char *path, char *buf, size_t size, off_t off,
	  struct fuse_file_info *fi)
{
	lionfile_t *file;
	size_t ret = 0;

	/* we can't proceed if path is not a fakefile */
	if (strncmp(path, "/.ff/", 5) != 0)
		return -ENOENT;
	path += 4;

	/* if file does not exist we can't proceed */
	pthread_rwlock_rdlock(&files_lock); /* list read lock */
	if ((file = get_file_by_path(path)) == NULL) {
		pthread_rwlock_unlock(&files_lock);
		return -ENOENT;
	}

	pthread_rwlock_rdlock(&file->lock); /* file read lock */
	pthread_rwlock_unlock(&files_lock);

	if (off < file->size) {
		if(off + size > file->size)
			size = file->size - off;
	} else {
		pthread_rwlock_unlock(&file->lock);
		return 0;
	}

	ret = network_file_get_data(file->url, size, off, buf);

	pthread_rwlock_unlock(&file->lock);

	return ret;
}

static int
lion_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
	     struct fuse_file_info *fi)
{
	lionfile_t *file;
	int i;

	/* doesn't path equals "/" */
	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	pthread_rwlock_rdlock(&files_lock); /* list read lock */

	list_for_each_entry(file, &files, list_entry)
		filler(buf, file->path + 1, NULL, 0);

	pthread_rwlock_unlock(&files_lock);

	return 0;
}

static struct fuse_operations fuseopr = {
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

	// init list rwlock
	pthread_rwlock_init(&files_lock, NULL);

	// init list
	INIT_LIST_HEAD(&files);

	// init network
	network_init();

	// open all network modules available
	network_open_all_modules();

	// Main routine. It initializes FUSE and set the operations (&fuseopr)
	fuse_main(argc, argv, &fuseopr, NULL);

	// close all network modules
	network_close_all_modules();

	// destroy rwlock
	pthread_rwlock_destroy(&files_lock);

	return ret;
}
