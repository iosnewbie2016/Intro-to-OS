#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gfserver.h"

#define DEFAULT_SOURCE "s3.amazonaws.com/content.udacity-data.com"

/* This is the proxy usage string */
static void usage(const char *source) {
  fprintf(stderr, "usage:\n"                                                              \
                  "\twebproxy [options]\n"                                                \
                  "options:\n"                                                            \
                  "\t-p [listen_port]    Listen port (Default: 6200)\n"                   \
                  "\t-s [source url]     The source for the data (Default: %s)\n"         \
                  "\t-t [thread_count]   Num worker threads (Default: 2, Range: 1-1024)\n"\
                  "\t-h                  Show this help message\n",
                  NULL == source ? DEFAULT_SOURCE : source);
}

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"server",        required_argument,      NULL,           's'},
  {"thread-count",  required_argument,      NULL,           't'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,            0}
};

extern ssize_t handle_with_file(gfcontext_t *ctx, char *path, void* arg);
extern ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg);
extern ssize_t handle_with_curl(gfcontext_t *ctx, char *path, void* arg);

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
  unsigned short nworkerthreads = 2;
  char *source = DEFAULT_SOURCE;
  int status = 0;

  /* disable buffering on stdout so it prints immediately */
  setbuf(stdout, NULL);

  if (signal(SIGINT, _sig_handler) == SIG_ERR){
    fprintf(stderr,"Can't catch SIGINT...exiting.\n");
    exit(SERVER_FAILURE);
  }

  if (signal(SIGTERM, _sig_handler) == SIG_ERR){
    fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
    exit(SERVER_FAILURE);
  }

  /* Parse and set command line arguments */
  while ((option_char = getopt_long(argc, argv, "p:s:t:xh", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      default:
        status = 1;
      case 'h': // help
        usage(DEFAULT_SOURCE);
        exit(status);
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
      case 'x': // experimental
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


  /* initialize the server structure, specify number of threads */ 
  gfserver_init(&gfs, nworkerthreads);
  // fprintf(stderr, "%s\n", "Proxy Server Initialized");
  /* set server options */
  gfserver_setopt(&gfs, GFS_PORT, port);
  gfserver_setopt(&gfs, GFS_MAXNPENDING, 12);
  gfserver_setopt(&gfs, GFS_WORKER_FUNC, handle_with_curl);

  // fprintf(stderr, "%s\n", "Spinning Up Worker Threads");
  for(i = 0; i < nworkerthreads; i++) {
    gfserver_setopt(&gfs, GFS_WORKER_ARG, i, source);
  }
  
  /* Invoke the framework.  This runs the server */
  /* Note: this loops forever */
  gfserver_serve(&gfs);

  /* not reached */
  return -1;
}
