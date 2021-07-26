// lionfs, The Link Over Network File System
// Copyright (C) 2021  Ricardo Biehl Pasquali <pasqualirb@gmail.com>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

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
	size_t tmp(void *src, size_t size, size_t nmemb, void *dst) { memcpy(dst, src, size * nmemb); }
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, tmp);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, data); // passed as dst in the above memcpy()

	// Do the request
	ret = curl_easy_perform(uri);
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
	return 0;
}

int
get_info(lionfile_info_t *info, char *uri)
{
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
	if (ret != CURLE_OK)
		goto error;

	// Do the request
	ret = curl_easy_perform(uri);
	if (ret != CURLE_OK)
		goto error;

	// Get file size
	ret = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &info->size);
	if (ret != CURLE_OK)
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
