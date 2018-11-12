#include "magickminify.h"
#include "minifyjpeg.h"

/* Implement the server-side functions here */

bool_t minifyjpeg_proc_1_svc(image_in image_input, image_out *result, struct svc_req *ptr){
	ssize_t src_len, dst_len;
	// void* magickminify(void* src, ssize_t src_len, ssize_t* dst_len);
	src_len = image_input.src.src_len;
	// or ssize_t src_len = image_input.src_len?
	// printf("Thread ID: %ld started", pthread_self());
	// printf("File source length = %lu\n", src_len);
	result->dst.dst_val = magickminify(image_input.src.src_val, src_len, &dst_len);
	result->dst.dst_len = (u_int) dst_len;
	// printf("Downsampled\n");
	// 1 indicates success to the calling routine, 0 indicates failure.
	// TODO: Error check
	return 1;
}

int minifyjpeg_prog_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result){
	magickminify_cleanup();
	xdr_free(xdr_result, result);
	return 1;
}