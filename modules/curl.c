// lionfs, The Link Over Network File System
// Copyright (C) 2021  Ricardo Biehl Pasquali <pasqualirb@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "common.h"

static int is_curl_initialized = 0;
static void
ensure_curl_initialized()
{
	if (!is_curl_initialized) {
		curl_global_init(CURL_GLOBAL_DEFAULT);
		is_curl_initialized = 1;
	}
}

static size_t
copy_helper(void *src, size_t size, size_t nmemb, void *dst)
{
	if (dst)
		memcpy(dst, src, size * nmemb);
	return size * nmemb;
}

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
	ensure_curl_initialized();

	CURL *curl = curl_easy_init();
	if (!curl)
		return -1;

	int ret = 0;

	// Set URL
	ret = curl_easy_setopt(curl, CURLOPT_URL, uri);
	if (ret != CURLE_OK)
		goto error;

	// Set the portion of the file to get
	char range[64];
	snprintf(range, 64, "%lld-%lld", off, (off + size) - 1);
	ret = curl_easy_setopt(curl, CURLOPT_RANGE, range);
	if (ret != CURLE_OK)
		goto error;

	// Set the callback when request finishes
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, copy_helper);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, data); // passed as dst in memcpy()

	// Do the request
	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK)
		goto error;

cleanup:
	curl_easy_cleanup(curl);
	return ret;

error:
	ret = -ret;
	goto cleanup;
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
	ensure_curl_initialized();
	return 0;
}

int
get_info(lionfile_info_t *info, char *uri)
{
	ensure_curl_initialized();

	CURL *curl = curl_easy_init();
	if (!curl)
		return -1;

	int ret = 0;

	// Set URL
	ret = curl_easy_setopt(curl, CURLOPT_URL, uri);
	if (ret != CURLE_OK)
		goto error;

	// Set the option for returning size and last-mofified time
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

	// Do the request
	ret = curl_easy_perform(curl);
	if (ret != CURLE_OK)
		goto error;

	// Get file size
	ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &info->size);
	if (ret != CURLE_OK || info->size < 1)
		goto error;

	// Get file last-modified time
	ret = curl_easy_getinfo(curl, CURLINFO_FILETIME, &info->mtime);
	if (ret != CURLE_OK)
		goto error;

cleanup:
	curl_easy_cleanup(curl);
	return ret;

error:
	ret = -ret;
	goto cleanup;
}
