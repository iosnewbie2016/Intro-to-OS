#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Added lib
#include <arpa/inet.h>

/* Be prepared accept a response of this length */
// I'm confused it says minimum of 16 bytes
#define BUFSIZE 256
#define PORT "6200"

#define USAGE                                                                       \
    "usage:\n"                                                                      \
    "  echoclient [options]\n"                                                      \
    "options:\n"                                                                    \
    "  -s                  Server (Default: localhost)\n"                           \
    "  -p                  Port (Default: 6200)\n"                                  \
    "  -m                  Message to send to server (Default: \"hello world.\")\n" \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

// TCP Client
// getaddrinfo()
// socket()
// connect() TCP Connection Establishment - not bind we dont care about local port
// write() data(request)
// read() data(reply)
// close()

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int option_char = 0;
    char *hostname = "localhost";
    unsigned short portno = 6200; // default port
    char *message = "Hello world!!";

    // Clean This Up:
    int status;
    // sockfd is the listen() -server the new one would be for the single connection
    int len, bytes_sent;
    int sockfd, numbytes;     // handle the socket file descriptor 
    // why use hints and not struct sockaddr_in servaddr?
    // servaddr is the old way and getaddrinfo() is easier also allows IPv6
    struct addrinfo hints, *servinfo, *p; // parameter with relevant info for getaddrinfo()
    // struct addrinfo *servinfo; // will point to the results
    // struct addrinfo *p; // to use to loop through the results
    char recvbuf[BUFSIZE]; // max text line length
    // char strHostName[INET6_ADDRSTRLEN];
    char ipstr[INET6_ADDRSTRLEN];
    char port[100] = PORT;
    //char portNum[6]; 

    /**********************************************************************/

    // Parse and set command line arguments
    while ((option_char = getopt_long(argc, argv, "s:p:m:hx", gLongOptions, NULL)) != -1)
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
        case 'm': // message
            message = optarg;
            break;
        case 'h': // help
            fprintf(stdout, "%s", USAGE);
            exit(0);
            break;
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }

    if (NULL == message)
    {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */

    // Basic check of the arguments
    // additional checks can be inserted
    if (argc > 7) {
        fprintf(stderr, "%s", USAGE);
        exit(1);
    }

    // Create a Socket
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    // get ready to connect

    // I think I can add the following but it does not matter
    // hints.ai_protocol = 0 // 0 for Any
    // Might have to deal with a hostname that is a number?

    if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    // loop through all the results and connect to the first we can
    p = servinfo;
    while (p != NULL) {

       sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
       
       if(sockfd == -1) {
            fprintf(stderr, "Problem in creating the socket");
            continue;
       }

        // returns 0 if it succeeds in establishing a connection
        if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("client: connect");
            close(sockfd); // close the socket
            continue;
        } else {
            break;
        }

        p = p->ai_next;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    void *addr;
    // NOT USED
    // char *ipver;

    // get the pointer to the address itself,
    // different fields in IPv4 and IPv6
    if(p->ai_family == AF_INET) {
        // IPv4
        struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
        addr = &(ipv4->sin_addr);
        //ipver = "IPv4";
    } else {
        // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        addr = &(ipv6->sin6_addr);
        //ipver = "IPv6";
    }

    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    // printf("client: connecting to %s: %s\n", ipver, ipstr);

    freeaddrinfo(servinfo); // all done with this structure

    // Assume the full message is sent or received in every send or recv call
    // look at the return value and simply exit if the full message is not sent
    // or received. Not necessarily null-terminated. 
    len = strlen(message);
    if ((bytes_sent = send(sockfd, message, len, 0)) == -1) {
        fprintf(stderr, "send went wrong\n");
        exit(1);
    }

    if (bytes_sent != len) {
        fprintf(stderr, "Full message not sent!\n");
        exit(1);
    }

    if ((numbytes = recv(sockfd, recvbuf, BUFSIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    recvbuf[numbytes] = '\0';

    printf("%s",recvbuf);
    close(sockfd);
    return 0;
}
