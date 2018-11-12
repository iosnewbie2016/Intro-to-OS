/*
 * Please do not make me edit this file any more.
 * It was generated using rpcgen.
 */

#include "magickminify.h"
#include "minifyjpeg.h"
#include "steque.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <getopt.h>
#include <errno.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

pthread_mutex_t mylock; // global mutex lock
pthread_t p_thread;
pthread_attr_t attr;
steque_t workqueue;

// Create a thread argument struct for the func()
typedef struct {
	struct svc_req *rqstp;
	SVCXPRT *transp;
	image_in minifyjpeg_proc_1_arg;
} thread_data;

static void usage() {
	fprintf(stderr, "usage:\n"
					"server [options]\n"
					"options:\n"
					"\t-t [thread_count] Num worker threads (Default: 2, Range: 1-1024)\n"
					"\t-h                Show this help message\n");
}

/* OPTIONS DESCRIPTOR ==============================================*/
static struct option gLongOptions[] = {
	{"thread-count", required_argument, NULL, 't'},
	{"help",  		 no_argument,	    NULL, 'h'},
	{NULL, 			 0, 				NULL,   0}
};

int
_minifyjpeg_proc_1 (image_in  *argp, void *result, struct svc_req *rqstp)
{
	// printf("inside WORK \n");
	return (minifyjpeg_proc_1_svc(*argp, result, rqstp));
}
// Put an object in queue after svc_arg which has to be picked up by a worker.
// This is my handler. First struct is used for the switch statement
// The second argument is used to extract the input data with svc_getargs() and 
// to send the output data with svc_sendreply().
// Calling svc_getargs in the main (boss) thread 
void *worker(void *arg) {
	thread_data *data_ptr;
	// printf("inside worker before loop \n");
	while(1) {
		pthread_mutex_lock(&mylock);
		while(!steque_isempty(&workqueue)) {
			// printf("inside worker inside loop 1 \n");
			data_ptr = steque_pop(&workqueue);
    	pthread_mutex_unlock(&mylock);

			if (data_ptr == NULL) {
				fprintf(stderr, "dequeue() error\n");
				exit(1);
			}

			union {
				image_out minifyjpeg_proc_1_res;
			} result;

			xdrproc_t _xdr_result;
			_xdr_result = (xdrproc_t) xdr_image_out;

			int aRetval = _minifyjpeg_proc_1(&data_ptr->minifyjpeg_proc_1_arg, &result, data_ptr->rqstp);
			// printf("Hm. %d\n", aRetval);


			if (!svc_sendreply(data_ptr->transp, (xdrproc_t) _xdr_result, (char *)&result)) {
				svcerr_systemerr (data_ptr->transp);
		}

			// printf("inside worker inside loop 7 \n");
			if (!minifyjpeg_prog_1_freeresult (data_ptr->transp, _xdr_result, (caddr_t) &result))
				fprintf (stderr, "%s", "unable to free results");

			free(data_ptr);
			pthread_mutex_lock(&mylock);
		}
		pthread_mutex_unlock(&mylock);
	}

	// printf("after worker loop \n");

	return NULL;
}

// Boss thread
static void
minifyjpeg_prog_1(struct svc_req *rqstp, register SVCXPRT *transp)
{	
	// printf("Inside boss.\n");
	thread_data *data_ptr;
	data_ptr = (thread_data*)malloc(sizeof(thread_data));
	
	union {
			image_in minifyjpeg_proc_1_arg;
	} argument;
	union {
		image_out minifyjpeg_proc_1_res;
	} result;
	bool_t retval;

	memset ((char *)&argument, 0, sizeof (argument));
	xdrproc_t _xdr_argument, _xdr_result;
	_xdr_argument = (xdrproc_t) xdr_image_in;

	// Break up the function that calls svc_getargs between the boss and the worker function
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}

	data_ptr->rqstp = rqstp;
	data_ptr->transp = transp;
	data_ptr->minifyjpeg_proc_1_arg = argument.minifyjpeg_proc_1_arg;

	// printf("data_ptr: %s : %lu", data_ptr->transp, data_ptr->rqstp);

	steque_enqueue(&workqueue, data_ptr);
	// printf("Size of the queue: %d\n", steque_size(&workqueue));
	// worker(NULL);
	// free(data_ptr);

	// if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
	// 	fprintf (stderr, "%s", "unable to free arguments");
	// 	exit (1);
	// }

	// printf("Leaving boss.\n");
	return;
}

int
main (int argc, char **argv)
{
	/* Parse and set command line arguments */
	int option_char = 0;
	int status = 0;
	unsigned short nworkerthreads = 2;

	while ((option_char = getopt_long(argc, argv, "t:h", gLongOptions, NULL)) != -1) {
		switch (option_char) {
			default:
				status = 1;
			case 'h':
				usage();
				exit(status);
				break;
			case 't': // thread-count
				nworkerthreads = atoi(optarg);
				break;
		}
	}

	if (nworkerthreads > 1024) {
		fprintf(stderr, "Worker thread count is invalid: %d\n", nworkerthreads);
		usage();
		exit(__LINE__);
	}

	// printf("Creating mutex:\n");
	pthread_mutex_init(&mylock, NULL);

	// printf("Number of workers: %d\n", nworkerthreads);

	// Create a threadpool containing the number of threads
	// Do I init magickminify here? NOOOOO, never init
	pthread_t *pthreads;
	pthread_attr_t attr;
	pthreads = (pthread_t *) malloc(nworkerthreads * sizeof(pthread_t));
    // Initialize the queue
    // Figure out how to load the queue in a loop within the boss
	steque_init(&workqueue);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	for(int i = 0; i < nworkerthreads; i++) {
		// printf("Creating worker: %d\n", i+1);
		if(pthread_create(&pthreads[i], NULL, worker, NULL)) {
			fprintf(stderr, "Error creating thread\n");
			return -1;
		}
  	}

  	register SVCXPRT *transp;

	pmap_unset (MINIFYJPEG_PROG, MINIFYJPEG_VERS);
	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.");
		exit(1);
	}
	if (!svc_register(transp, MINIFYJPEG_PROG, MINIFYJPEG_VERS, minifyjpeg_prog_1, IPPROTO_UDP)) {
		fprintf (stderr, "%s", "unable to register (MINIFYJPEG_PROG, MINIFYJPEG_VERS, udp).");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, MINIFYJPEG_PROG, MINIFYJPEG_VERS, minifyjpeg_prog_1, IPPROTO_TCP)) {
		fprintf (stderr, "%s", "unable to register (MINIFYJPEG_PROG, MINIFYJPEG_VERS, tcp).");
		exit(1);
	}


  	/*
  	for(int i=0; i < nworkerthreads; i++) {
  		pthread_join(pthreads)
  	}
	*/

	svc_run ();
	// printf("Not reached right now\n");
	fprintf (stderr, "%s", "svc_run returned");
	exit (1);
	/* NOTREACHED */
}
