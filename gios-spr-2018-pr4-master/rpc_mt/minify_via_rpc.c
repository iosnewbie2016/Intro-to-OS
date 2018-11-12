#include "minifyjpeg_clnt.c"
#include "minifyjpeg_xdr.c"
#include <stdio.h>
#include <stdlib.h>

CLIENT* get_minify_client(char *server){
	// For the timeout issue:
	// https://docs.oracle.com/cd/E19253-01/816-1435/rpcgenpguide-21/index.html
    CLIENT *cl;
    struct timeval tv;

    /* Your code goes here */
    cl = clnt_create(server, MINIFYJPEG_PROG, MINIFYJPEG_VERS, "tcp");
    if (cl == (CLIENT *)NULL) {
    	clnt_pcreateerror(server);
    	exit(1);
    }
  
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    // printf("%lu\n", tv.tv_sec);
    clnt_control(cl, CLSET_TIMEOUT, &tv);
	
    return cl;
}

/*
The size of the minified file is not known before the call to the library that minimizes it,
therefore this function should allocate the right amount of memory to hold the minimized file
and return it in the last parameter to it
*/
int minify_via_rpc(CLIENT *cl, void* src_val, size_t src_len, size_t *dst_len, void **dst_val){

	/*Your code goes here */
    // have a malloc call for struct image_out *dst_len = malloc(sizeof(*dst_len)); ?
    struct image_in image_input;
    struct image_out *image_output;
    enum clnt_stat clnt_stat;

    image_input.src.src_val = src_val;
    image_input.src.src_len = src_len;

    // printf("File len of source is %u\n\n", image_input.src.src_len);

    // allocate the right amount of mem to hold the min file and the param
    image_output = malloc(sizeof(image_out));
    image_output->dst.dst_val = malloc(src_len * sizeof(char));
    clnt_stat = minifyjpeg_proc_1(image_input, image_output, cl);
    if (image_output == NULL) {
    	clnt_perror(cl, "minifyjpeg");
    	exit(1);
    }

    *dst_len = image_output->dst.dst_len;
    // printf("Downsampled Image Size: %lu\n", *dst_len);

    // I think the right side of the equation is a char*
    *dst_val = image_output->dst.dst_val;

    // clnt_destroy(cl);
    // free(image_output->dst.dst_val);
    free(image_output);
    if (clnt_stat != RPC_SUCCESS) {
    	clnt_perror(cl, "minifyjpeg");
    }
    // clnt_destroy(cl);
 	return ((int)clnt_stat);
}

/* Routine for freeing memory that may be allocated in the server procedure 
 * https://docs.oracle.com/cd/E19683-01/816-1435/rpcgenpguide-24243/index.html
 */
int minifyjpeg_prog_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result) {
	xdr_free(xdr_result, result);
	return 1;
}
