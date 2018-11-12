#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>

#include "gfserver.h"
#include "gfserver-student.h"

#define BUFSIZE 16384

struct gfcontext_t {
	int socket_fd;
	size_t filelen; 		// gfc_get_filelen
	size_t bytessent; 	// gfc_get_bytesreceived
    // char* header;
    // size_t headerlen;
};

struct gfserver_t {
	char* port; // store the port gfs_set_port
	void *arg; // gfs_set_handlearg
	// handler will read the file specified in the path
	// then use sendheader, send, and abort functions
	ssize_t (*handler)(gfcontext_t *, char *, void *); // gfs_set_handler
	int max_npending; // gfs_set_maxnpending
};

/* 
 * Modify this file to implement the interface specified in
 * gfserver.h.
 */

/*
 * Aborts the connection to the client associated with the input
 * gfcontext_t.
 */
void gfs_abort(gfcontext_t *ctx){
	// close the socket, free any resources allocated 
	//printf("SERVER - ABORTING CONTEXT %d\n",ctx->socket_fd);
	close(ctx->socket_fd);
	free(ctx);
}

gfserver_t* gfserver_create(){
	// Set defaults for anything set?
    return (gfserver_t *) malloc(sizeof(gfserver_t));
}

/*
 * Sends size bytes starting at the pointer data to the client 
 * This function should only be called from within a callback registered 
 * with gfserver_set_handler.  It returns once the data has been
 * sent.
 */
ssize_t gfs_send(gfcontext_t *ctx, void *data, size_t len){
	// Use gfcontext
	// int send_fd;
	int bytes_sent;
	if ((bytes_sent = send(ctx->socket_fd, data, len, 0)) == -1) {
		fprintf(stderr, "send went wrong\n");
		gfs_abort(ctx);
	}
	// printf("SERVER - Just sent %d\n", bytes_sent);
	ctx->bytessent = ctx->bytessent+bytes_sent;
	if(ctx->bytessent >= ctx->filelen && bytes_sent <= 0) {
		printf("SERVER - Done sending the file, aborting.\n");
		gfs_abort(ctx);
	}
    return bytes_sent;
}

/*
 * Sends to the client the Getfile header containing the appropriate 
 * status and file length for the given inputs.  This function should
 * only be called from within a callback registered gfserver_set_handler.
 */
ssize_t gfs_sendheader(gfcontext_t *ctx, gfstatus_t status, size_t file_len){
    // Use gfcontext
    // handler would support only OK status while calling gfs_sendheader()

    // Build the header
    char* header_scheme = "GETFILE";
    char* header_status;
    char* file_length;
    char* header;
    switch (status) {
    	case GF_OK: 
    		header_status = "OK";
    		break;
    	case GF_FILE_NOT_FOUND:
    		header_status = "FILE_NOT_FOUND";
    		break;
        case GF_ERROR: 
        	header_status = "ERROR";
        	break;
        case GF_INVALID:
        	header_status = "INVALID";
        	break;
    }
    if (strcmp(header_status,"INVALID") != 0) {
    	file_length = malloc(sizeof(file_len));
    	printf("SERVER - file_len: %zu\n", file_len);
    	// Convert file length to a char*
    	snprintf(file_length, sizeof(file_len), "%zu", file_len);
    	printf("SERVER -File Length: %s\n", file_length);
    	char* header_terminator = "\r\n\r\n";
    	header = malloc(sizeof(header_scheme) + sizeof(header_status) + 
    		sizeof(file_length) + sizeof(header_terminator)+3*sizeof(char));
    	sprintf(header, "%s %s %s%s", header_scheme, header_status, 
    		file_length, header_terminator);
    	printf("SERVER - Header response to send: %s\n", header); 
    } else {
    	file_length = NULL;
    	printf("HEADER SCHEME: %zu\n", strlen(header_scheme));
    	printf("STATUS: %zu\n", strlen(header_status));
    	char* header_terminator = "\r\n\r\n";
    	header = malloc(sizeof(header_scheme) + sizeof(header_status)+sizeof(header_terminator)+2*sizeof(char));
    	sprintf(header, "%s %s", header_scheme, header_status);
    	printf("SERVER - Header response to send: %s\n", header); 
    }

    // int numBytes_sent;
    int headerlen;
    int retval;

    headerlen = strlen(header);
    // TODO: Implement gfs_send
    ctx->filelen = file_len;
    ctx->bytessent = 0;
    if ((retval = gfs_send(ctx, header, headerlen)) == -1) {
    	fprintf(stderr, "Header sent went wrong\n");
    	free(file_length);
    	free(header);
    	gfs_abort(ctx);
    }
	printf("SERVER - Just sent header size: %d\n", retval);
	free(file_length);
	free(header);
    return retval;
}

// Helper function: get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
 * Starts the server.  Does not return.
 */
void gfserver_serve(gfserver_t *gfs){
	// Forever loop
	// Listens
	// Accepts
	// Read in the request
	// Validate it and parse out the file path
	// Call handler with file path and stored handler args

	/* If the request is ERROR or INVALID
	 * the getfile_handler is never called and then 
	 * abort the connection and freeing ctx
	 *
	 * If the request is good, getfile_handler is called TODO!!!!
	 * assuming that get_content returns -1. 
	 * Status should be FILE_NOT_FOUND
	 *
	 * Just pass gfcontext to the functions that do something with gfcontext_t
	 * 
	 */
	for (;;) {

		int listen_sockfd, conn_sockfd; // listen: sockfd; connection: conn_sockfd
		int numBytes_read;
		char buffer[BUFSIZE];
		char ipstr[INET6_ADDRSTRLEN];

		struct addrinfo hints, *servinfo, *p;
		struct sockaddr_storage cliaddr; // connector's address info
		socklen_t client_len;

		int status;
		int set_reuse_addr = 1; // ON == 1

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		if ((status = getaddrinfo(NULL, gfs->port, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			exit(1);
		}

		// loop through servinfo and connect
		p = servinfo;
		while (p != NULL) {
			// Create a Socket
			listen_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

			if (listen_sockfd == -1) {
				fprintf(stderr, "Problem in creating the socket");
				// TODO: Figure out the error
				continue;
			}

			// Use wildcards to avoid conflicts
        	if (setsockopt(listen_sockfd,SOL_SOCKET,SO_REUSEADDR,&set_reuse_addr,
            	sizeof set_reuse_addr) == -1) {
            	fprintf(stderr, "server failed to set SO_REUSEADDR socket option\n");
            	// TODO: Figure out the error
            	exit(1);
        	}

        	printf("SERVER-Binding to port %s\n", gfs->port);
        	if(bind(listen_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            	close(listen_sockfd);
            	fprintf(stderr, "server failed to bind\n");
            	continue;
        	} else {
            	break; 
			}
			p = p->ai_next;
		}

		freeaddrinfo(servinfo); // done with the linked list

		// Check arguments and some error checking
		if (p == NULL) {
			fprintf(stderr, "server: failed to bind\n");
			// TODO: Figure out the error
			exit(1);
		}

		if (listen(listen_sockfd, gfs->max_npending) == -1) {
			fprintf(stderr, "listen: connection interrupted");
			// TODO: Figure out the error
			exit(1);
		}

		printf("SERVER -%s\n", "server running...waiting for a connection");

		for(;;) {
			client_len = sizeof cliaddr;
			conn_sockfd = accept(listen_sockfd, (struct sockaddr *) &cliaddr, &client_len);
			printf("SERVER -Received request...\n");

			if (conn_sockfd == -1) {
				fprintf(stderr, "accept had an error");
				continue;
			}

			inet_ntop(cliaddr.ss_family, get_in_addr((struct sockaddr *) &cliaddr),
				ipstr, sizeof ipstr);
			printf("SERVER -server: got connection from %s\n", ipstr);


			// TODO: Read the request
			if ((numBytes_read = recv(conn_sockfd, buffer, BUFSIZE,0)) == -1) {
				fprintf(stderr, "Read Request Error");
				exit(1);
			}

			if (buffer[0] == '\0') {
				fprintf(stderr, "Null string received");
				exit(1);
			}
			printf("SERVER -string received from the client:\n");
			printf("SERVER -%s\n", buffer);

			
			char* request_scheme;
			char* request_method;
			char* request_path;
			char * not_buf = malloc(BUFSIZE);
			strcpy(not_buf, buffer);
			request_scheme = strtok(not_buf, " ");
			request_method = strtok(NULL, " ");
			request_path = strtok(NULL, "\r\n\r\n");

			printf("SERVER -Scheme: %s\n", request_scheme);
			printf("SERVER -Method: %s\n", request_method);
			printf("SERVER -Path: %s\n", request_path);

			if(request_path[strlen(request_path)-1]==' ') { //CHECK FOR THAT EVIL AND PESKY OPTIONAL SPACE
				request_path[strlen(request_path)-1]='\0';//REMOVE IT!!!
			}
			// Validate the Request
			// When the request from the client is malformed 
			// Status to be returned is GF_INVALID

			// TODO: Setup gfcontext??
			gfcontext_t * context = malloc(sizeof(gfcontext_t)+4);
			context->socket_fd = conn_sockfd;

			if(strcmp(request_scheme, "GETFILE") != 0) {
				printf("MALFORMED HEADER\n");
				char* message = "GETFILE INVALID\r\n\r\n";
				send(context->socket_fd, message, strlen(message)+1, 0);
				// gfs_sendheader(context, GF_INVALID, 0);
				gfs_abort(context);
			} else if(strcmp(request_method, "GET") != 0) {
				printf("MALFORMED HEADER\n");
				char* message = "GETFILE INVALID\r\n\r\n";
				send(context->socket_fd, message, strlen(message)+1, 0);
				gfs_abort(context);
			} else if(request_path[0]!='/') {
				printf("MALFORMED HEADER\n");
				char* message = "GETFILE INVALID\r\n\r\n";
				send(context->socket_fd, message, strlen(message)+1, 0);
				gfs_abort(context);
			} else {
				printf("Last character-1 is: ~%c~\n",request_path[strlen(request_path)-1]);
				printf("SERVER -Calling the handler.\n");
				gfs->handler(context, request_path, gfs->arg);				
			}
		}
	}
}

void gfserver_set_handlerarg(gfserver_t *gfs, void* arg){
	gfs->arg = arg;
}

/*
 * Sets the handler callback, a function that will be called for each each
 * request.  As arguments, this function receives:
 * - a gfcontext_t handle which it must pass into the gfs_* functions that 
 * 	 it calls as it handles the response.
 * - the requested path
 * - the pointer specified in the gfserver_set_handlerarg option.
 * The handler should only return a negative value to signal an error.
 */
void gfserver_set_handler(gfserver_t *gfs, ssize_t (*handler)(gfcontext_t *, char *, void*)){
	// handler(gfcontext_t *ctx, path, gfs->arg);
	// Do not have to call the send_header() and send() funcitons directly since
	// the header.o file will do that
	// Call the handler from *serve and pass it the data it needs
	gfs->handler = handler;
}

/*
 * Sets the maximum number of pending connections which the server
 * will tolerate before rejecting connection requests.
 */
void gfserver_set_maxpending(gfserver_t *gfs, int max_npending){
	// default max_npending is max_request_len?
	// parallel to listenq
	printf("SERVER -Server Max Backlog: %d\n", max_npending);
	gfs->max_npending = max_npending;
}

/*
 * Sets the port at which the server will listen for connections.
 */
void gfserver_set_port(gfserver_t *gfs, unsigned short port){
	// convert port from unsigned short to string
	char portnum[sizeof(unsigned int)*8+1];
	sprintf(portnum, "%u", port);
	printf("SERVER -Connecting to port: %s\n", portnum);
	gfs->port = portnum;
}