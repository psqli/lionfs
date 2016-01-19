#include <stdlib.h>
#include <sys/mman.h>

#include "lonfs.h"
#include "cache.h"
#include "cache_array.h"

#if 0
/* mmap example miss length/fd/offset */
mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
#endif

static int fdlist[MAX_FILES];

static inline int
get_fd_by_fid(int fid)
{
	return fdlist[fid];
}

/*
 * cache_routine() Main cache routine executed as thread.
 * It get Cache Requests every moment and write each one into
 * cache file for that fid in `cacher->fid`.
 */
_routine_
cache_routine(void)
{
	CacheR *cacher;

	for(;;)
	{
		cacher = array_get();

		if(get_binfo(cacher->fid)[cacher->offset] == 1)
		{
			DEBUG("FATAL: replacing block %d", cacher->offset);
			free(cacher);
			return NULL;
		}

		write_data(get_fd_by_fid(cacher->fid), cacher->data,
	cacher->length, cacher->offset);

		get_binfo(cacher->fid)[cacher->offset] = 1;

		free(cacher);
	}
}

int
cache_data_insert(int fid, void *data, size_t length, off_t offset)
{
	if(get_binfo(fid)[offset] == 1)
		return -EEXIST;

	CacheR *cacher;

	if((cacher = malloc(sizeof(CacheR))) == NULL)
	{
		DEBUG("FATAL malloc (memory allocation) error!");
		return -1;
	}

	cacher->fid = fid;
	cacher->data = data;
	cacher->length = length;
	cacher->offset = offset;

	array_add(cacher);

	return 0;
}

void*
cache_data_request(int fid, off_t offset)
{}

int
cache_reset(int fid, size_t blocks)
{
	if(get_binfo(fid) == NULL || !blocks)
		return -EINVAL;

	free(get_binfo(fid));

	if(set_binfo(fid, malloc(blocks)) < 0)
		return -ENOMEM;

	return 0;
}

int
cache_init(void)
{
	/* CREATE PHYSICAL FILES FOR CACHING AND PUT ITS fd'S IN `fdlist` */
}
