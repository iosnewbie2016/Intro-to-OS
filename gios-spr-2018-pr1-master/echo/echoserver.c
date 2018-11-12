#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

// Added lib
#include <arpa/inet.h>

// Constants
#define BUFSIZE 16

#define LISTENQ 10 // How many pending connections queue will hold

#define PORT "6200"

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoserver [options]\n"                                                    \
"options:\n"                                                                  \
"  -p                  Port (Default: 6200)\n"                                \
"  -m                  Maximum pending connections (default: 1)\n"            \
"  -h                  Show this help message\n"                              \

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
  {"port",          required_argument,      NULL,           'p'},
  {"maxnpending",   required_argument,      NULL,           'm'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};

// Helper function: get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv) {
  int option_char;
  char port[100] = PORT;
  int portno = 6200; /* port to listen on */
  int maxnpending = 1;
  
  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "p:m:hx", gLongOptions, NULL)) != -1) {
   switch (option_char) {
      case 'p': // listen-port
        strcpy(port, optarg);
        portno = atoi(optarg);
        break;                                        
      default:
        fprintf(stderr, "%s ", USAGE);
        exit(1);
      case 'm': // server
        maxnpending = atoi(optarg);
        break; 
      case 'h': // help
        fprintf(stdout, "%s ", USAGE);
        exit(0);
        break;
    }
  }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535)) {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__, portno);
        exit(1);
    }
    if (maxnpending < 1) {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__, maxnpending);
        exit(1);
    }


  /* Socket Code Here */
    // TCP Server
    // getaddrinfo()
    // socket()
    // bind()
    // listen()
    // accept()

    // Initialize the variables
    int listen_sockfd, conn_sockfd; // listen on sock_fd, new connection on new_fd

    char buffer[BUFSIZE];
    int numBytes_read = 0;

    struct addrinfo hints, *servinfo, *p;
    // struct sockaddr_in servaddr;
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

    // Change portno to a str done with a buffer

    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
      return 1;
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    // loop through all the results and connect to the first we can
    p = servinfo;
    while(p != NULL) {
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
        perror("server: bind");
        continue;
      } else {
        break;
      }

      p = p->ai_next;
    }

    freeaddrinfo(servinfo); // done with the linked list

    if (p == NULL) {
      fprintf(stderr, "server: failed to bind\n");
      exit(1);
    }

    if(listen(listen_sockfd, LISTENQ) == -1) {
      perror("listen");
      exit(1);
    }
    printf("%s\n", "Server running...waiting for a connections");

    for (;;) {
      // get the connected socket
      client_len = sizeof (cliaddr);
      conn_sockfd = accept(listen_sockfd, (struct sockaddr *) &cliaddr, &client_len);
      printf("Received request...\n");

      if (conn_sockfd == -1) {
        perror("accept");
        continue;
      }

      inet_ntop(cliaddr.ss_family, get_in_addr((struct sockaddr *) &cliaddr),
        ipstr, sizeof ipstr);
      printf("server: got connection from %s\n", ipstr);

      numBytes_read = recv(conn_sockfd, buffer, BUFSIZE-1,0);
      if (numBytes_read < 0) {
        perror("Read error");
        exit(1);
      }
      printf("string received from and resent to the client:\n");
      // Print just BUFSIZE wide
      printf("%s\n", buffer);

      if (buffer[0] == '\0') {
        perror("Null string received");
        exit(1);
      }
      send(conn_sockfd, buffer, numBytes_read, 0);
      close(conn_sockfd);

      //Clear the buffer
      memset(&buffer, 0, sizeof buffer);
    }
}
