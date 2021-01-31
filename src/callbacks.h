#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <cstdio>

/* custom callback function for CURLOPT_WRITEFUNCTION. */
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream);

/*
 * Custom callback function for CURLOPT_HEADERFUNCTION to extract header data.
 * Currently only used to get the filename if available.
 */
size_t header_callback(char *buffer, size_t size, size_t nitems,
		       void *userdata);

/* custom callback function for CURLOPT_PROGRESSFUNCTION to create a progress bar. */
int progress_callback(void *ptr, double total_to_download, double now_downloaded,
		      double total_to_upload, double now_uploaded);

#endif
