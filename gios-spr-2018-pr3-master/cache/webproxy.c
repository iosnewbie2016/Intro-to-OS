#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include "gfserver.h"
#include "cache-student.h"

#define DEFAULT_SOURCE "s3.amazonaws.com/content.udacity-data.com"

/* This is the proxy usage string */
static void usage(const char *source) {
  fprintf(stderr, "usage:\n"                                                              \
                  "\twebproxy [options]\n"                                                \
                  "options:\n"                                                            \
                  "\t-n [segment_count]  Number of segments to use (Default: 3)\n"        \
                  "\t-p [listen_port]    Listen port (Default: 6200)\n"                   \
                  "\t-s [source url]     The source for the data (Default: %s)\n"         \
                  "\t-t [thread_count]   Num worker threads (Default: 3, Range: 1-1024)\n"\
                  "\t-z [segment_size]   The segment size (in bytes, Default: 6200)\n"    \
                  "\t-h                  Show this help message\n",
                  NULL == source ? DEFAULT_SOURCE : source);
}



/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"segment-count", required_argument,      NULL,           'n'},
  {"port",          required_argument,      NULL,           'p'},
  {"thread-count",  required_argument,      NULL,           't'},
  {"server",        required_argument,      NULL,           's'},
  {"segment-size",  required_argument,      NULL,           'z'},         
  {"help",          no_argument,            NULL,           'h'},
  {"hidden",        no_argument,            NULL,           'i'}, /* server side */
  {NULL,            0,                      NULL,            0}
};

extern ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg);

static gfserver_t gfs;

static void _sig_handler(int signo){
  if (signo == SIGINT || signo == SIGTERM){
    gfserver_stop(&gfs);
    exit(signo);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv) {
  int i;
  int option_char = 0;
  unsigned short port = 6200;
  unsigned short nworkerthreads = 3;
  unsigned int nsegments = 3;
  size_t segsize = 6200;
  char *source = DEFAULT_SOURCE;
  int status = 0;

  /* disable buffering on stdout so it prints immediately */
  setbuf(stdout, NULL);

//TODO: Clean up and properly exit by removing all IPC objects
  if (signal(SIGINT, _sig_handler) == SIG_ERR) {
    fprintf(stderr,"Can't catch SIGINT...exiting.\n");
    exit(SERVER_FAILURE);
  }

  if (signal(SIGTERM, _sig_handler) == SIG_ERR) {
    fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
    exit(SERVER_FAILURE);
  }

  /* Parse and set command line arguments */
  while ((option_char = getopt_long(argc, argv, "in:p:s:t:z:xh", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        status = 1;
      case 'h': // help
        usage(DEFAULT_SOURCE);
        exit(status);
      case 'n': // segment count
        nsegments = atoi(optarg);
        break;   
      case 'p': // listen-port
        port = atoi(optarg);
        break;
      case 's': // file-path
        source = optarg;
        break;                                          
      case 't': // thread-count
        nworkerthreads = atoi(optarg);
        break;
      case 'z': // segment size
        segsize = atoi(optarg);
        break;
      case 'i':
      case 'x': // experimental
        /* bonnie side only */
        break;
    }
  }

 if ((nworkerthreads < 1) || (nworkerthreads > 1024)) {
    fprintf(stderr, "Worker thread count is invalid %d\n", nworkerthreads);
    usage(NULL);
    exit(__LINE__);
  }

  if (port < 1024) {
    fprintf(stderr, "Port number is invalid %d\n", port);
    usage(NULL);
    exit(__LINE__);
  }

  if (NULL == source) {
    fprintf(stderr, "Invalid (null) source name\n");
    usage(NULL);
    exit(__LINE__);
  }

  if (segsize < 128) {
    fprintf(stderr, "Segment size is invalid (%zu)\n", segsize);
    usage(DEFAULT_SOURCE);
    exit(__LINE__);
  }

  if (nsegments < 1) {
    fprintf(stderr, "Must have a positive number of segments (%d)\n", nsegments);
    usage(DEFAULT_SOURCE);
    exit(__LINE__);
  }
  // Catching a signal:
  // https://www.thegeekstuff.com/2012/03/catch-signals-sample-c-code/
  

  /* Initialize shared memory HERE */
  size_t total_mm_size;
  void *addr;
  int fd;
  /* Creating a shared memory object whose size is specified by a command-line
  * argument and maps the object into the process's virtual address space */
  /* Create a new memory object */

  // USE SHM_OPEN WITH THE SAME NAME IN THE PROCESSES YOU WANT IPC. 
  // ONLY PASS THE SEGMENT NAMES
  const char *name = "temp";

  fd = shm_open(name, O_CREAT | O_RDWR, 0770);
  if (fd == -1) {
    fprintf(stderr, "Open failed: %s\n", strerror(errno));
    exit(1);
  }

  /* Set memory object size */
  // Multiply segsize and nsegments to get an overall pool?
  // Other option is just use segsize
  // Should I make this a global variable?
  total_mm_size = nsegments * segsize;

  // printf("Size %d\n",(int) total_mm_size);
  
  if (ftruncate(fd, total_mm_size) == -1) {
    fprintf(stderr, "ftruncate: %s\n", strerror(errno));
    exit(1);
  }

  /* Map the memory object */
  addr = mmap(0, total_mm_size, PROT_READ | PROT_WRITE, 
    MAP_SHARED, fd, 0);

  if (addr == MAP_FAILED) {
    fprintf(stderr, "mmap failed: %s\n", strerror(errno));
    exit(1);
  }

  // I think I initialize both message queues here....

  key_t key, key_cache_to_proxy;
  int msqid;

  if((key = ftok("/tmp", 'b')) == -1) {
    perror("ftok");
    _sig_handler(1);
  }

  if((key_cache_to_proxy = ftok(".", 'a')) == -1) {
    perror("Proxy ftok: cache to proxy");
    exit(1);
  }

  if ((msqid = msgget(key, 0666 | IPC_CREAT)) == -1) {
    perror("msgget");
    exit(1);
  }


  /* Initialize handle_with_cache HERE */
  // Pass the fd to handle_with_cache to pass to simplecached?

  /* This is where you initialize the server struct */
  gfserver_init(&gfs, nworkerthreads);

   /* set server options */
  gfserver_setopt(&gfs, GFS_PORT, port);
  gfserver_setopt(&gfs, GFS_MAXNPENDING, 42);
  gfserver_setopt(&gfs, GFS_WORKER_FUNC, handle_with_cache);
  for(i = 0; i < nworkerthreads; i++) {
    gfserver_setopt(&gfs, GFS_WORKER_ARG, i, name);
  }
  
  /* Invoke the framework.  This runs the server */
  /* Note: this loops forever */
  gfserver_serve(&gfs);

  return 0;
}
