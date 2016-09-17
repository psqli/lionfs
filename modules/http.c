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
#include <string.h>

#include "../lib/libghttp/ghttp.h"

#include "common.h"

/**
 * get_data() Read from a file pointed by URI over network.
 *
 * @p data Pointer where to store read data.
 * @p uri 'http://' URI to a file over network.
 * @p off Read offset.
 * @p size Read size.
 */
size_t
get_data(void *data, char *uri, long long off, size_t size)
{
	if(size == 0)
		return 0;

	ghttp_request *ghr;

	char range[64];

	size_t ret = 0;

	ghr = ghttp_request_new();

	if(ghttp_set_uri(ghr, uri) == -1)
		goto _go_request_destroy;

	ghttp_set_type(ghr, ghttp_type_get);

	ghttp_set_sync(ghr, ghttp_sync);

	/* Make the value for range request header */
	snprintf(range, 64, "bytes=%lld-%lld", off, (off + size) - 1);

	ghttp_set_header(ghr, "Range", range);

	ghttp_prepare(ghr);

	if(ghttp_process(ghr) != ghttp_done)
		goto _go_request_destroy;

	if(ghttp_status_code(ghr) != 206)
		goto _go_request_destroy;

	if((ret = ghttp_get_body_len(ghr)) > size)
	{
		ret = 0;
		goto _go_request_destroy;
	}

	memcpy(data, ghttp_get_body(ghr), ret);

_go_request_destroy:
	ghttp_request_destroy(ghr);

	return ret;
}

/**
 * get_valid() Validate an URI and check if it support range requests. Return 0
 * if OK or 1 if FAILED.
 *
 * @p uri 'http://' URI to a file over network.
 */
int
get_valid(char *uri)
{
	ghttp_request *ghr;

	const char *accept_ranges;

	int ret = 1;

	ghr = ghttp_request_new();

	if(ghttp_set_uri(ghr, uri) == -1)
		goto _go_request_destroy;

	ghttp_set_type(ghr, ghttp_type_head);

	ghttp_set_sync(ghr, ghttp_sync);

	// ghttp_set_header(ghr, "Range", "bytes=0-0");

	ghttp_prepare(ghr);

	if(ghttp_process(ghr) != ghttp_done)
		goto _go_request_destroy;

	if(ghttp_status_code(ghr) != 200)
		goto _go_request_destroy;

	if((accept_ranges = ghttp_get_header(ghr, "Accept-Ranges")) == NULL)
		goto _go_request_destroy;

	if(strcmp(accept_ranges, "bytes"))
		goto _go_request_destroy;

	ret = 0;

_go_request_destroy:
	ghttp_request_destroy(ghr);

	return ret;
}

int
get_info(lionfile_info_t *info, char *uri)
{
	ghttp_request *ghr;

	char *date_string;

	int ret = -1;

	ghr = ghttp_request_new();

	if(ghttp_set_uri(ghr, uri) == -1)
		goto _go_request_destroy;

	ghttp_set_type(ghr, ghttp_type_head);

	ghttp_set_sync(ghr, ghttp_sync);

	ghttp_prepare(ghr);

	if(ghttp_process(ghr) != ghttp_done)
		goto _go_request_destroy;

	if(ghttp_status_code(ghr) != 200)
		goto _go_request_destroy;

	if((info->size = atoll(ghttp_get_header(ghr, "Content-Length"))) == 0)
		goto _go_request_destroy;

	date_string = (char*) ghttp_get_header(ghr, "Last-Modified");
	info->mtime = ghttp_parse_date(date_string);

	ret = 0;

_go_request_destroy:
	ghttp_request_destroy(ghr);

	return ret;
}
