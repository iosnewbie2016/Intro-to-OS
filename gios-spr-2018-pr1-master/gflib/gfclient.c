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

#include "gfclient.h"
#include "gfclient-student.h"

#define BUFSIZE 4096

struct gfcrequest_t {
	char* server; 			// gfc_set_server
	char* path; 			// gfc_set_path
	unsigned short port; 	// gfc_set_port
	void (*headerfunc)(void *, size_t, void *); 		// gfc_set_headerfunc
	void *headerarg; 		// gfc_set_headerarg
	void (*writefunc)(void*, size_t, void *);		// gfc_set_writefunc
	void *writearg; 		// gfc_set_writearg
	gfstatus_t status;		// gfc_get_status
	size_t filelen; 		// gfc_get_filelen
	size_t bytesreceived; 	// gfc_get_bytesreceived
};

void gfc_cleanup(gfcrequest_t *gfr){
	free(gfr);
}

gfcrequest_t *gfc_create(){ // First thing when not multhithreading (or when spwaning child threads?)
	//TODO: Initialize bytesreceived to 0?
	//TODO: Initialize filelen to 0?
    return (gfcrequest_t *) malloc(sizeof(gfcrequest_t));
}

size_t gfc_get_bytesreceived(gfcrequest_t *gfr){
    return gfr->bytesreceived;
}

size_t gfc_get_filelen(gfcrequest_t *gfr){
    return gfr->filelen;
}

gfstatus_t gfc_get_status(gfcrequest_t *gfr){
    return gfr->status;
}

void gfc_global_init(){ // First thing when multithreading
}

void gfc_global_cleanup(){
}

int gfc_perform(gfcrequest_t *gfr){
	/* ==========================================================================
 * TCP Client
 * Client simply connects to the server
 * getaddrinfo()
 * socket()
 * connect()
 * read()
 * save_file()
 * close()
 * Server will send a file back to the client
 * The Client will save the file locally
 ===========================================================================*/

	// Constants 
    int status; // 
    int sockfd;
    int headerlen;
    int bytes_sent;
    int numbytes = 1;

    // will have relevant info, point to the results, loop through results
    struct addrinfo hints, *servinfo, *p;

    char recvbuf[BUFSIZE];
    char ipstr[INET6_ADDRSTRLEN];
    char portnum[sizeof(unsigned int)*8+1];
    sprintf(portnum, "%u", gfr->port);

    // Create a Socket
    memset(&hints, 0, sizeof hints); // struct is empty
    hints.ai_family = AF_UNSPEC; // IP agnostic
    hints.ai_socktype = SOCK_STREAM; // TCP Stream Sockets

    if ((status = getaddrinfo(gfr->server, portnum, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    p = servinfo;
    while(p != NULL) {
 		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if(sockfd == -1) {
            fprintf(stderr, "client failed to create socket\n");
            continue;
        }

        // Returns 0 if it succeeds in establishing a connection
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            fprintf(stderr, "client failed to connect to %s:%s!\n", gfr->server, portnum);
            close(sockfd);
            continue;
        } else {
            break;
        }

        p = p->ai_next;
    }

    if (p == NULL) {
        fprintf(stderr, "client failed to connect to %s:%s!\n", gfr->server, portnum);
        close(sockfd);
        return -1;
    }

    void *addr;
    char *ipver;

    // get the pointer to the address itself, IP agnostic
    if(p->ai_family == AF_INET) {
        // IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
        addr = &(ipv4->sin_addr);
        ipver = "IPv4";
    } else {
        // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        addr = &(ipv6->sin6_addr);
        ipver = "IPv6";
    }

    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("client: connecting to %s: %s\n", ipver, ipstr);

    freeaddrinfo(servinfo);

    // Build the header
    char *header_scheme = "GETFILE";
    char *header_method = "GET";
    char *header_path = gfr->path;
    char *header_terminator = "\r\n\r\n";
    char *header = malloc(strlen(header_scheme) + strlen(header_method) +
    	strlen(header_path) + strlen(header_terminator)+3*sizeof(char));

    sprintf(header, "%s %s %s%s", header_scheme, header_method, 
    	header_path, header_terminator);

    printf("%s\n", header);

    gfr->headerfunc(header, strlen(header), gfr->headerarg);

    headerlen = strlen(header);
    if ((bytes_sent = send(sockfd, header, headerlen, 0)) == -1) {
        fprintf(stderr, "send went wrong\n");
        free(header);
        return -1;
    }

    free(header);

    if (bytes_sent != headerlen) {
        fprintf(stderr, "Full message not sent!\n");
        return -1;
    }

    // Header is sent!
    // Wait for response.

    // Get bytes from file
    // use memset to clear the buffer
    memset(recvbuf, 0, BUFSIZE);
    if ((numbytes = recv(sockfd, recvbuf, BUFSIZE, 0)) == -1) {
        perror("recv");
        return -1;
    }

    // Now recvbuf has the first "packet"
    // printf("%s\n", recvbuf);
    // Need to parse the header:
    char * response_scheme;
    char * response_status;
    char * response_length;
    char * not_buf = malloc(BUFSIZE);
    strcpy(not_buf, recvbuf);
    response_scheme = strtok(not_buf, " ");
    response_status = strtok(NULL, " ");
    response_length = strtok(NULL, "\r\n\r\n");

    printf("SCHEME: %s\nSTATUS: %s\nLENGTH: %s\n", response_scheme, response_status, response_length);

    if (response_status == NULL || strcmp(response_status,"INVALID\r\n\r\n") == 0) {
    	gfr->status = GF_INVALID;
    	return -1;
    }

    if (strcmp(response_status,"FILE_NOT_FOUND\r\n\r\n") == 0) {
    	gfr->status = GF_FILE_NOT_FOUND;
    	return 0;
    }

	if (strcmp(response_status,"ERROR\r\n\r\n") == 0) {
    	gfr->status = GF_ERROR;
    	return 0;
    }    

    if (response_length == NULL){
    	gfr->status = GF_INVALID; //TODO: Make sure this is always the case.
    	printf("response length null");
    	return -1;
    }

    unsigned long response_length_ul = strtoul(response_length, NULL, 10);

    printf("LENGTH UNSIGNED LONG: %lu\n", response_length_ul);

    gfr->status = GF_OK; 

    gfr->filelen = (size_t) response_length_ul;
    gfr->bytesreceived = numbytes - (strlen(response_scheme) + strlen(response_status) + strlen(response_length) + strlen("\r\n\r\n")+2);

    if (gfr->filelen < BUFSIZE) {
    	if (gfr->filelen > gfr->bytesreceived) {
    		gfr->status = GF_OK;
    		printf("filelen is not the same as bytesreceived");
    		return -1;
    	}
    	char subbuf[gfr->bytesreceived];
    	memcpy(subbuf, &recvbuf[(strlen(response_scheme) + strlen(response_status) + strlen(response_length) + strlen("\r\n\r\n")+2)], gfr->filelen);
    	gfr->writefunc(subbuf, gfr->bytesreceived, gfr->writearg);
    	free(not_buf);
    	return 0;
    }

    gfr->writefunc(&recvbuf[(strlen(response_scheme) + strlen(response_status) + strlen(response_length) + strlen("\r\n\r\n")+2)], gfr->bytesreceived, gfr->writearg);
    free(not_buf);

    while(numbytes > 0 && gfr->filelen > gfr->bytesreceived) {
        memset(recvbuf, 0, BUFSIZE);
        if ((numbytes = recv(sockfd, recvbuf, BUFSIZE, 0)) == -1) {
        	gfr->status = GF_ERROR;
            fprintf(stderr, "recv\n");
            return -1;
        }
        gfr->bytesreceived += numbytes;

        //printf("Num Bytes received: %d\n", numbytes);
        gfr->writefunc(recvbuf, (size_t) numbytes, gfr->writearg); 
    }


    close(sockfd);

    if (gfr->filelen != gfr->bytesreceived) {
    	gfr->status = GF_OK;
    	printf("filelen is not the same as bytesreceived");
    	return -1;
    }

    printf("SUCCESS");
    return 0;
}

void gfc_set_headerarg(gfcrequest_t *gfr, void *headerarg){ // Set the path.
	gfr->headerarg = headerarg;
}

void gfc_set_headerfunc(gfcrequest_t *gfr, void (*headerfunc)(void*, size_t, void *)){ //
	gfr->headerfunc = headerfunc;
}

void gfc_set_path(gfcrequest_t *gfr, char* path){
	gfr->path = path;
}

void gfc_set_port(gfcrequest_t *gfr, unsigned short port){
	gfr->port = port;
}

void gfc_set_server(gfcrequest_t *gfr, char* server){
	gfr->server = server;
}

void gfc_set_writearg(gfcrequest_t *gfr, void *writearg){
	gfr->writearg = writearg;
}

void gfc_set_writefunc(gfcrequest_t *gfr, void (*writefunc)(void*, size_t, void *)){
	gfr->writefunc = writefunc;
}

char* gfc_strstatus(gfstatus_t status){
    switch(status) {
        case GF_OK: return "GF_OK";
        case GF_FILE_NOT_FOUND: return "GF_FILE_NOT_FOUND";
        case GF_ERROR: return "GF_ERROR";
        case GF_INVALID: return "GF_INVALID";
    } 

    return "GF_ERROR";
}
