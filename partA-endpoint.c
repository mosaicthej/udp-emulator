#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* getaddrinfo */
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>

#define handle_error_en(en, msg)                                               \
  do {                                                                         \
    errno = en;                                                                \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define PORTMAX 65535
#define PORTMIN 1024

/* arguments to each thread. */
struct sender_info {
  char *send_to_host;
  char *send_to_port;
};

struct receiver_info {
  char *receive_from_port;
};

/* validate port numbers */
int check_args(char *st_host, char *st_port, char *rf_port) {
  int st_p = atoi(st_port);
  int rf_p = atoi(rf_port);

  if (strlen(st_host) == 0) {
    fprintf(stderr, "send-to-host cannot be empty\n");
    return EXIT_FAILURE;
  }

  if (st_p < PORTMIN || st_p > PORTMAX) {
    fprintf(stderr, "send-to-port must be between %d and %d\n", PORTMIN,
            PORTMAX);
    return EXIT_FAILURE;
  }

  if (rf_p < PORTMIN || rf_p > PORTMAX) {
    fprintf(stderr, "receive-from-port must be between %d and %d\n", PORTMIN,
            PORTMAX);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

  }
}

int send_thread(char *send_to_host, char *send_to_port) {
  /* create a socket */
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }

  /* set up the address to send to */
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  int status = getaddrinfo(send_to_host, send_to_port, &hints, &res);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return EXIT_FAILURE;
  }

  /* send data */
  char *msg = "Hello, world!";
  int numbytes =
      sendto(sockfd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
  if (numbytes < 0) {
    perror("sendto");
    return EXIT_FAILURE;
  }

  freeaddrinfo(res);
  close(sockfd);
  return EXIT_SUCCESS;
}


