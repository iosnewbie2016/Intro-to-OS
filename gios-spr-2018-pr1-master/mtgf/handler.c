#include <stdlib.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "gfserver.h"
#include "content.h"

#include "gfserver-student.h"

#define BUFFER_SIZE 6200

//
//  The purpose of this function is to handle a get request
//
//  The ctx is the "context" operation and it contains connection state
//  The path is the path being retrieved
//  The arg allows the registration of context that is passed into this routine.
//  Note: you don't need to use arg. The test code uses it in some cases, but
//        not in others.
//
ssize_t getfile_handler(gfcontext_t *ctx, char *path, void* arg){
	return -ENOSYS;
}

