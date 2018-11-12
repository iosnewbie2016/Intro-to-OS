#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <printf.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/signal.h>
#include <unistd.h>

#include "gfserver.h"
#include "proxy-student.h"

#define BUFSIZE (6200)

// Define a struct for accepting LCs output
struct BufferStruct {
	char * buffer;
	size_t size;
};

// This is the function we pass to LC, which writes the output to a BufferStruct s
// size is the size of one data item and nmemb is the number of data items
static size_t WriteMemoryCallback (void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct BufferStruct * mem = (struct BufferStruct *) data;

	mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);

	if (mem->buffer) {
		memcpy( &( mem->buffer[mem->size]), ptr, realsize);
		mem->size = mem->size + realsize;
		mem->buffer[mem->size] = 0;
	}

	return realsize;
}
/*
 * handle_with_curl is the suggested name for your curl call back.
 * We suggest you implement your code here.  Feel free to use any
 * other functions that you may need.
 */
ssize_t handle_with_curl(gfcontext_t *ctx, char *path, void* arg){
	(void) ctx;
	(void) path;
	(void) arg;
	// Using void to supress compiler warnings
	// curl_global_init gets called once globally and subsequent calls
	// to curl_easy_init are guaranteed to be thread-safe
	// Make sure to clean up during error handling
	// Saving to memory would be a bad idea 
	// fprintf(stderr, "%s\n", "Using curl");
	/* From handle_with_file */
	// int fildes;
	size_t bytes_transferred;
	ssize_t write_len;
	char buffer[BUFSIZE];
	char *data_dir = arg;
	// struct stat statbuf;

	strncpy(buffer, data_dir, BUFSIZE);
	strncat(buffer, path, BUFSIZE);

	// printf("Using Data Directory: %s\n", data_dir);
	// printf("Path: %s\n", path);

	CURL *curl_handle;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	struct BufferStruct output; // create an instance of out BufferStruct to accept LCs
	output.buffer = NULL;
	output.size = 0;


	// init the curl session 
	curl_handle = curl_easy_init();

	// specify URL to get 
	if (curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, buffer);

		// send all data to this function 
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

		// we pass our 'chunk' struct to the callback function 
		// could def pass it to ctx
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&output);

		// some servers don't like requests that are made without a user-agent field,
		// so we provide one 
		curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

		// send the header first with the file size
		res = curl_easy_perform(curl_handle);
		double cl;

		if(!res) {
			res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl);
			if(res == CURLE_OK) {
				printf("Size: %.0f\n", cl);
				// printf("Sending header first\n");
				if (cl < 0) {
					return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
				}
				gfs_sendheader(ctx, GF_OK, cl);
			}
			else {
				// fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				return SERVER_FAILURE;
			}
		}

		// curl_easy_cleanup(curl_handle);
		bytes_transferred = 0;
		while(bytes_transferred < cl) {
			// Read
			// res = curl_easy_perform(curl_handle);

			// check for errors 
			if (res != CURLE_OK) {
				// fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				return SERVER_FAILURE;
			} else {
				printf("%lu bytes received\n", (long) output.size);
				write_len = gfs_send(ctx, output.buffer, output.size);
				printf("%lu bytes sent\n", (long) write_len);
			}

			bytes_transferred += write_len;
		}

	}
	
	// cleanup curl stuff 
	curl_easy_cleanup(curl_handle);

	free(output.buffer);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	return bytes_transferred;
}


/*
 * The original sample uses handle_with_file.  We add a wrapper here so that
 * you can use this in place of the existing invocation of handle_with_file.
 * 
 * Feel free to use handle_with_curl directly.
 */
ssize_t handle_with_file(gfcontext_t *ctx, char *path, void* arg){
	return handle_with_curl(ctx, path, arg);
}