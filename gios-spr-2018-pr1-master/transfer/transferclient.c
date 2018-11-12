#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <getopt.h>
#include <arpa/inet.h>

#define BUFSIZE 256
#define PORT "6200"

#define USAGE                                                \
    "usage:\n"                                               \
    "  transferclient [options]\n"                           \
    "options:\n"                                             \
    "  -s                  Server (Default: localhost)\n"    \
    "  -p                  Port (Default: 6200)\n"           \
    "  -o                  Output file (Default 6200.txt)\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

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
/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 6200;
    char *filename = "6200.txt";
    char port[100] = PORT;

    setbuf(stdout, NULL);

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:o:hx", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            strcpy(port,optarg);
            portno = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s", USAGE);
            exit(1);
        case 'o': // filename
            filename = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == filename)
    {
        fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
        exit(1);
    }

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    /* Socket Code Here */

    // Constants 
    int status; // 
    int sockfd;
    int numbytes;

    // will have relevant info, point to the results, loop through results
    struct addrinfo hints, *servinfo, *p;

    char recvbuf[BUFSIZE];
    char ipstr[INET6_ADDRSTRLEN];

    FILE *received_file;
    

    // Basic check of arguments
    if (argc > 7) {
        fprintf(stderr, "%s", USAGE);
        exit(1);
    }

    // Create a Socket
    memset(&hints, 0, sizeof hints); // struct is empty
    hints.ai_family = AF_UNSPEC; // IP agnostic
    hints.ai_socktype = SOCK_STREAM; // TCP Stream Sockets

    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    // loop through all the results and connect to the first we can
    // I could get rid of this loop to differentiate it
    p = servinfo;
    while(p != NULL) {

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if(sockfd == -1) {
            fprintf(stderr, "client failed to create socket\n");
            continue;
        }

        // Returns 0 if it succeeds in establishing a connection
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            fprintf(stderr, "client failed to connect to %s:%s!\n", hostname, port);
            close(sockfd);
            continue;
        } else {
            break;
        }

        p = p->ai_next;
    }

    if (p == NULL) {
        fprintf(stderr, "client failed to connect to %s:%s!\n", hostname, port);
        close(sockfd);
        exit(1);
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

    freeaddrinfo(servinfo); // all done

    // Get bytes from file
    // use memset to clear the buffer
    memset(recvbuf, 0, BUFSIZE);
    if ((numbytes = recv(sockfd, recvbuf, BUFSIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    printf("Opened file: %s\n", filename);
    // printf("Num Bytes received: %d\n", numbytes);
    // Write the buffer to a file
    received_file = fopen(filename, "w");
    if (received_file == NULL) {
        fprintf(stderr, "Failed to open file %s\n", strerror(errno));
        exit(1);
    }
    fwrite(recvbuf, sizeof(char), numbytes, received_file);
    // printf("%s\n", recvbuf);

    while(numbytes > 0) {
        memset(recvbuf, 0, BUFSIZE);
        if ((numbytes = recv(sockfd, recvbuf, BUFSIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        // printf("Num Bytes received: %d\n", numbytes);
        fwrite(recvbuf, sizeof(char), numbytes, received_file);
        // printf("%s\n", recvbuf);  
    }

    fclose(received_file);

    close(sockfd);
    return 0;
}
