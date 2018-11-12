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
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>

#include "cache-student.h"
#include "gfserver.h"
#include "shm_channel.h"
#include "simplecache.h"


#if !defined(CACHE_FAILURE)
#define CACHE_FAILURE (-1)
#endif // CACHE_FAILURE

#define MAX_CACHE_REQUEST_LEN 6200
#define BUFSIZE (6200)

#define DEFAULT_CACHE "./cached_files"

sem_t command_mutex, response_mutex;

int msqid, msqid_cache_to_proxy;
key_t key, key_cache_to_proxy;

static void _sig_handler(int signo){
    if (signo == SIGINT || signo == SIGTERM || signo == SIGSEGV){
        printf("\nReceived signo = %d in sig_handler()\n", signo);
        /* Unlink IPC mechanisms here*/
        const char *name = "temp";
        if (msgctl(msqid, IPC_RMID, NULL) == -1) {
            perror("msgctl");
        }

        if (msgctl(msqid_cache_to_proxy, IPC_RMID, NULL) == -1) {
            perror("msgctl: cache to proxy");
        }
        shm_unlink(name);
        // for(i = 0; i < nthreads; i++) {
        //   pthread_join(ptr[i], NULL);
        // }
        sem_destroy(&command_mutex);
        sem_destroy(&response_mutex);
        exit(signo);
    }
}

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  simplecached [options]\n"                                                  \
"options:\n"                                                                  \
"  -c [cachedir]       Path to static files (Default: ./)\n"                  \
"  -t [thread_count]   Thread count for work queue (Default: 3, Range: 1-1024)\n"      \
"  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"cachedir",           required_argument,      NULL,           'c'},
  {"nthreads",           required_argument,      NULL,           't'},
  {"help",               no_argument,            NULL,           'h'},
  {"hidden",			 no_argument,			 NULL,			 'i'}, /* server side */
  {NULL,                 0,                      NULL,             0}
};

void Usage() {
  fprintf(stdout, "%s", USAGE);
}

struct msgbuf {
	long mtype;
	char buffer[BUFSIZE];
};

void* worker(void* arg) {

	int cache_fd;
	struct msgbuf mybuf, mybuf_cache_to_proxy;
	size_t file_len, bytes_transferred;
	ssize_t read_len;
	char *path_name;
	char *shm_name;
	struct stat fileStat;

		for (;;) {
		// printf("Waiting for commands!\n");
		wait:
		sem_wait(&command_mutex);

		// printf("MQ for (C -> P): %d\n", msqid_cache_to_proxy);
		// printf("MQ for (P -> C): %d\n", msqid);

		// printf(msgrcv(msqid, &mybuf, sizeof mybuf.buffer, 0, 0))
		if (msgrcv(msqid, &mybuf, sizeof mybuf.buffer, 0, 0) == -1) { //msgctl(msqid, IPC_STAT, )
			perror("msgrcv");
			simplecache_destroy();
			exit(CACHE_FAILURE);
		}
		sem_post(&command_mutex);

		// printf("Message: \"%s\"\n", mybuf.buffer);
		char *path = mybuf.buffer;
		path_name = strtok_r(path, " ", &path);
		shm_name  = strtok_r(NULL, "", &path);
		mybuf_cache_to_proxy.mtype = 2;

		if ((cache_fd = simplecache_get(path_name)) < 0) {
			// File Not Found, how do I handle this?
			// mybuf_cache_to_proxy.mtype = 2;
			char str[50];
			strncpy(str, "NOT_FOUND", 50);
			strncpy(mybuf_cache_to_proxy.buffer,str,BUFSIZE);
			int len = strlen(mybuf_cache_to_proxy.buffer);
			if (msgsnd(msqid_cache_to_proxy, &mybuf_cache_to_proxy, len+1, 0) == -1) {
				perror("msgsend: cache to proxy");
				fprintf(stderr, "%s\n", strerror(errno));
				exit(CACHE_FAILURE);
			}
			goto wait;
		}

		// printf("Cache file descriptor: %d\n", cache_fd);

		/* Calculate the file size */
		if (0 > fstat(cache_fd, &fileStat)) {
			printf("Reache Here\n");
			char str[50];
			strncpy(str, "NOT_FOUND", 50);
			strncpy(mybuf_cache_to_proxy.buffer,str,BUFSIZE);
			int len = strlen(mybuf_cache_to_proxy.buffer);
			printf("Sending: %s\n", mybuf_cache_to_proxy.buffer);
			if (msgsnd(msqid_cache_to_proxy, &mybuf_cache_to_proxy, len+1, 0) == -1) {
				perror("msgsend (fuck you 2): cache to proxy");
				exit(CACHE_FAILURE);
			}
			printf("Reache Here 2\n");
			if (msgctl(msqid_cache_to_proxy, IPC_RMID, NULL) == -1) {
            	perror("msgctl: cache to proxy");
            	exit(CACHE_FAILURE);
        	}
        	printf("Reache Here 3\n");
			return &command_mutex; // TODO: handle this error.
		}
		printf("File Length: ");
		file_len = (size_t) fileStat.st_size;
		fprintf(stderr, "%zd\n", file_len);

		// Pass back the the file_len to use in gfs_sendheader
		
		char str[50];
		snprintf(str, 49, "%d\n", (int) file_len);
		// printf("Temp String: %s\n", str);
		strncpy(mybuf_cache_to_proxy.buffer,str,BUFSIZE);
		int len = strlen(mybuf_cache_to_proxy.buffer);

		// SHARED MEMORY TIME
		int shm_fd;
		char *addr;
		char file_buffer[BUFSIZE];
		// Don't create more shared memory?
		
		sem_wait(&response_mutex);
		shm_fd = shm_open(shm_name, O_RDWR, 0770);

		// printf("Shared Mem FD: %d\n", shm_fd);

		ftruncate(shm_fd, file_len);

		addr = mmap(0, file_len, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
		// printf("Addr: %p\n", addr);

		// Write to the shared memory in chunks
		// Needs explicit sync
		bytes_transferred = 0;
		while(bytes_transferred < file_len) {
			// printf("bytes_transferred: %d\n", (int) bytes_transferred);
			read_len = pread(cache_fd, file_buffer, BUFSIZE, bytes_transferred);
			if (read_len <= 0) {
				fprintf(stderr, "cache file read error\n");
				return &command_mutex; // TODO: handle this error.
			}
			// Might have to use memmove()
			memcpy(addr+bytes_transferred, file_buffer, read_len);
			bytes_transferred += read_len;
		}
	
		printf("Cache - bytes_transferred: %d\n\n", (int) bytes_transferred);
		// printf("Addr: %p\n\n", addr);

		//SEND THE ADDRESS AND FILE LENGTH TO THE PROXY

		// send it
		if (msgsnd(msqid_cache_to_proxy, &mybuf_cache_to_proxy, len+1, 0) == -1) {
			perror("msgsend (fuck you 3): cache to proxy");
			exit(CACHE_FAILURE);
		}
		sem_post(&response_mutex);

	}
	return &command_mutex;
}

int main(int argc, char **argv) {
	int nthreads = 3;
	char *cachedir = "locals.txt";
	char option_char;
	int status = 0;

	/* disable buffering to stdout */
	setbuf(stdout, NULL);

	while ((option_char = getopt_long(argc, argv, "ic:ht:x", gLongOptions, NULL)) != -1) {
		switch (option_char) {
			default:
				status = 1;
			case 'h': // help
				Usage();
				exit(status);
			case 'c': //cache directory
				cachedir = optarg;
				break;
			case 't': // thread-count
				nthreads = atoi(optarg);
				break;   
			case 'i': // server side usage
				break;
			case 'x': // experimental
				break;
		}
	}

	if ((nthreads>6200) || (nthreads < 1)) {
		fprintf(stderr, "Invalid number of threads\n");
		exit(__LINE__);
	}

	if (SIG_ERR == signal(SIGINT, _sig_handler)){
		fprintf(stderr,"Unable to catch SIGINT...exiting.\n");
		exit(CACHE_FAILURE);
	}

	if (SIG_ERR == signal(SIGTERM, _sig_handler)){
		fprintf(stderr,"Unable to catch SIGTERM...exiting.\n");
		exit(CACHE_FAILURE);
	}

	/* Cache initialization */
	simplecache_init(cachedir);

	/* Add your cache code here */

	// Message Queue Code goes here

	// Make message queue keys
	// Message Queue: Proxy to Cache
	if((key = ftok("/tmp", 'b')) == -1) {
		perror("ftok");
		simplecache_destroy();
		exit(CACHE_FAILURE);
	}

	// Read-Only Message Queue
	// Message Queue: Proxy to Cache
	while ((msqid = msgget(key, 0440)) == -1) {
		// perror("msgget");
		//simplecache_destroy();
		//exit(CACHE_FAILURE);
	}

	// Message Queue: Cache to Proxy
	if ((key_cache_to_proxy = ftok(".", 'a')) == -1) {
		perror("Cache-ftok: cache to proxy");
		simplecache_destroy();
		exit(CACHE_FAILURE);
	}

	// Create the new Message Queue
	// Message Queue: Cache to Proxy
	if ((msqid_cache_to_proxy = msgget(key_cache_to_proxy, 0666 | IPC_CREAT)) == -1) {
		perror("msgget: cache to proxy");
		simplecache_destroy();
		exit(CACHE_FAILURE);
	}

	sem_init(&command_mutex, 0, 1);
	sem_init(&response_mutex, 0, 1);
	// pthread_t t1, t2;
	// pthread_create(&t1, NULL, worker, NULL);
	// pthread_create(&t2, NULL, worker, NULL);

	pthread_t *ptr;
	ptr = malloc(sizeof(pthread_t)*nthreads);

	for(int i = 0; i < nthreads; i++) {
    	pthread_create(&ptr[i], NULL, worker, NULL);
  	}

	for(;;){

	}

	/* this code probably won't execute */
	return -1;
}
