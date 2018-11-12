/*
 * Define your service specification in this file and run rpcgen -MN minifyjpeg.x
 */
struct image_in {
	opaque src<>;
	int src_len;
	int dst_len;
};

struct image_out {
	opaque dst<>;
};

program MINIFYJPEG_PROG {
	version MINIFYJPEG_VERS {
		image_out MINIFYJPEG_PROC(image_in)=1; /*proc no=1*/
	} = 1;
} = 0x20000008;