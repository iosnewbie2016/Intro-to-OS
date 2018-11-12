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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/mman.h>

#include "gfserver.h"
#include "cache-student.h"

#define BUFSIZE (6200)

int msqid, msqid_cache_to_proxy;
key_t key, key_cache_to_proxy;

struct msgbuf {
	long mtype;
	char buffer[BUFSIZE];
};

ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg){
	struct msgbuf mybuf, mybuf_cache_to_proxy;
	// int msqid, msqid_cache_to_proxy;
	// key_t key, key_cache_to_proxy;
	char *name = arg;

	// Process ID and Thread ID
	// printf("Process ID: %d, Thread ID: %lu\n", getpid(), pthread_self());

	// Create the Message Queue from Proxy to Cache
	if((key = ftok("/tmp", 'b')) == -1) {
		perror("ftok");
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}
	
	if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
		perror("msgget");
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}

	mybuf.mtype = 1;
	strncpy(mybuf.buffer,path, BUFSIZE);
	strncat(mybuf.buffer," ", BUFSIZE);
	strncat(mybuf.buffer,name,BUFSIZE);

	int len = strlen(mybuf.buffer);

	// printf("%s\n", mybuf.buffer);

	if(msgsnd(msqid, &mybuf, len+1, 0) == -1) {
		perror("msgsnd");
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}

	// Connect to the Message Queue from Cache to Proxy
	if((key_cache_to_proxy = ftok(".", 'a')) == -1) {
		perror("Proxy ftok: cache to proxy");
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}

	while((msqid_cache_to_proxy = msgget(key_cache_to_proxy, 0440)) == -1) {

	}

	// printf("MQ for (C -> P): %d\n", msqid_cache_to_proxy);
	// printf("MQ for (P -> C): %d\n", msqid);

	if(msgrcv(msqid_cache_to_proxy, &mybuf_cache_to_proxy, sizeof mybuf_cache_to_proxy.buffer, 0, 0) == -1) {
		// perror("msgrcv: cache to proxy");
		// TODO: RIGHT NOW WE GET STUCK HERE IF cache IS NOT RUNNING.
		// printf("Message from cache 2: %s\n", mybuf_cache_to_proxy.buffer);
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}

	// printf("Message from cache: %s\n", mybuf_cache_to_proxy.buffer);

	if (strcmp(mybuf_cache_to_proxy.buffer, "NOT_FOUND") == 0) {
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}
	// TODO: SEND HEADER to CLIENT
	// 
	int message_from_cache = atoi(mybuf_cache_to_proxy.buffer);
	
	if (message_from_cache == -1) {
		printf("Reached HERE 2\n");
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	} 

	printf("File Length to be sent: %d\n", message_from_cache);

	gfs_sendheader(ctx, GF_OK, message_from_cache);
	
	int shm_fd;
	// char *addr;
	size_t file_len, bytes_transferred;
	ssize_t read_len, write_len;
	char file_buffer[BUFSIZE];

	file_len = atoi(mybuf_cache_to_proxy.buffer);
	shm_fd = shm_open(name, O_RDONLY, 0770);
	// addr = mmap(0, file_len, PROT_READ, MAP_SHARED, shm_fd, 0);
	
	bytes_transferred = 0;
	// printf("Addr: %p\n", addr);
	while(bytes_transferred < file_len) {
		// printf("bytes_transferred: %d\n", (int) bytes_transferred);
		read_len = pread(shm_fd, file_buffer, BUFSIZE, bytes_transferred);
		if (read_len <= 0) {
			fprintf(stderr, "proxy file read error\n");
			return -1;
		}
		write_len = gfs_send(ctx, file_buffer, read_len);
		if (write_len != read_len){
			fprintf(stderr, "bytes written does not equal bytes read\n");
			return -1;
		}
		bytes_transferred += read_len;
	}
	printf("Proxy - bytes_transferred: %d\n", (int) bytes_transferred);
	
	return bytes_transferred;
}

