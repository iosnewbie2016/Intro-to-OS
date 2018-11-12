#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFSIZE 4096
#define LISTENQ 10 // How many pending connections queue will hold
#define PORT "6200"

// Under this usage should I have maximum pending connections?
#define USAGE                                                \
    "usage:\n"                                               \
    "  transferserver [options]\n"                           \
    "options:\n"                                             \
    "  -f                  Filename (Default: cs6200.txt)\n" \
    "  -h                  Show this help message\n"         \
    "  -p                  Port (Default: 6200)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"filename", required_argument, NULL, 'f'},
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {NULL, 0, NULL, 0}};

// Helper function: get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
* int socket;
* char *buffer
* size_t length;
* ssize_t sent;
*
* Preparing message
*…
*
*Sending message
* sent = send(socket, buffer, length, 0);
*
* assert( sent == length); //WRONG!
* 
* A sender of data may also want to use this approach of sending it a chunk at a time
* Simply start by transferring data from a file over the network once it established a
* connection and closes the socket when finished. 
* 
*/

int main(int argc, char **argv)
{
    int option_char;
    int portno = 6200;             /* port to listen on */
    char *filename = "cs6200.txt"; /* file to transfer */
    char port[100] = PORT;

    setbuf(stdout, NULL); // disable buffering

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "p:hf:x", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 'p': // listen-port
            strcpy(port, optarg);
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        case 'f': // listen-port
            filename = optarg;
            break;
        }
    }


    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    
    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    // Constants
    int listen_sockfd, conn_sockfd; // listen: sockfd, new connection: conn_sockfd

    char buffer[BUFSIZE];
    // int numBytes_read = 0;

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage cliaddr; // connector's address information
    socklen_t client_len;
    char ipstr[INET6_ADDRSTRLEN];

    int status;
    int set_reuse_addr = 1; // ON == 1

    // Basic check of the arguments
    if (argc > 7) {
      fprintf(stderr, "%s", USAGE);
      exit(1);
    }

    // Use getaddrinfo to be IP agnostic and prepare the socket address
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    // loop through all the results and connect to the first we can
    p = servinfo;
    while(p != NULL) {
        // Create a Socket
        listen_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (listen_sockfd == -1) {
            fprintf(stderr, "Problem in creating the socket");
            continue;
        }

        // Use wildcards to avoid conflicts
        if (setsockopt(listen_sockfd,SOL_SOCKET,SO_REUSEADDR,&set_reuse_addr,
            sizeof set_reuse_addr) == -1) {
            fprintf(stderr, "server failed to set SO_REUSEADDR socket option\n");
            exit(1);
        }

        printf("Binding to port %d\n", portno);
        if(bind(listen_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_sockfd);
            fprintf(stderr, "server failed to bind");
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
        exit(1);
    }

    if(listen(listen_sockfd, LISTENQ) == -1) {
        fprintf(stderr, "listen: connection interrupted");
        exit(1);
    }
    printf("%s\n", "Server running...waiting for a connection");

    // Do an initial load to a buffer?
    // char *filename = "cs6200.txt"; /* file to transfer */
    FILE *transfer_file;
    int total_bytes; 
    int bytes_sent = 0;
    int bytes_left;
    int tempnum;
    struct stat st;

    printf("Reading File: %s\n", filename);
    if (stat(filename, &st) == 0) {
        total_bytes = st.st_size; 
        printf("Bytes Left: %jd\n", (intmax_t)total_bytes);
    } else {
        fprintf(stderr, "cannot determine the file size");
        exit(1);
    }
    bytes_left = total_bytes;

    transfer_file = fopen(filename, "rb"); // Open the file in binary mode
    fread(buffer, BUFSIZE-1, 1, transfer_file);
    // printf("Make sure everything is reading in: \n");
    // printf("%s\n", buffer);

    for (;;) {
        client_len = sizeof cliaddr;
        conn_sockfd = accept(listen_sockfd, (struct sockaddr *) &cliaddr, &client_len);
        printf("Received request...\n");

        if (conn_sockfd == -1) {
            fprintf(stderr, "accept had an error");
            continue;
        }

        inet_ntop(cliaddr.ss_family, get_in_addr((struct sockaddr *) &cliaddr),
            ipstr, sizeof ipstr);
        printf("server: got connection from %s\n", ipstr);


        if (bytes_left < BUFSIZE-1) {
            tempnum = send(conn_sockfd, buffer, bytes_left, 0);
        } else {
            tempnum = send(conn_sockfd, buffer, BUFSIZE-1, 0);
        }

        bytes_sent = bytes_sent + tempnum;
        bytes_left = bytes_left - tempnum;

        printf("Bytes sent: %d\n", bytes_sent);

        while (bytes_sent < total_bytes) {
            // Clear the buffer
            memset(&buffer, 0, sizeof buffer);

            // Then Send
            if (bytes_left < BUFSIZE-1) {
                fread(buffer, bytes_left, 1, transfer_file);
                tempnum = send(conn_sockfd, buffer, bytes_left, 0);
            } else {
                fread(buffer, BUFSIZE-1, 1, transfer_file);
                tempnum = send(conn_sockfd, buffer, BUFSIZE-1, 0);
            }
            printf("Number sent: %d\n", tempnum);
            if (tempnum == -1) {
                fprintf(stderr, "send error?");
                break;
            }
            bytes_sent += tempnum;
            printf("Updated bytes sent: %d\n", bytes_sent);
            bytes_left -= tempnum;
            printf("Number of bytes to send still: %d\n", bytes_left);
        }
        // Close the socket once everything was sent
        printf("total bytes sent: %d\n", bytes_sent);
        fclose(transfer_file);
        close(conn_sockfd);
        
    }
    // Close
    return(0);

}
