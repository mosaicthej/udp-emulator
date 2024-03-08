#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if UINTPTR_MAX == 0xffff /* 16-bit */
#define SYS_BITS 16
#define VOID_PTR_CAST uint16_t
#define INT_FMT "%d"
#elif UINTPTR_MAX == 0xffffffff /* 32-bit */
#define SYS_BITS 32
#define VOID_PTR_CAST uint32_t
#define INT_FMT "%d"
#elif UINTPTR_MAX == 0xffffffffffffffff /* 64-bit */
#define SYS_BITS 64
#define VOID_PTR_INT_CAST uint64_t
#define INT_FMT "%ld"
#else /* wtf */
#endif

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
  pthread_t thread_id; /* set by pthread_create, parent has access to */
  char *send_to_host;
  char *send_to_port;
};

struct receiver_info {
  pthread_t thread_id;
  char *receive_from_port;
};

VOID_PTR_INT_CAST send_thread(void *);
VOID_PTR_INT_CAST receive_thread(void *);

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

/*
 * Program: end points
 *   2 threads, 1 for sending, 1 for receiving
 *
 *   Send-to host:port are from command line arguments,
 *   receive-from port is also from command line argument.
 *
 *  using posix threads to do threading.
 * */

int main(int argc, char *argv[]) {
  int s; /* keep track status of system calls */

  char *send_to_host, *send_to_port, *receive_from_port; /* args */

  /* pthread related things */
  pthread_attr_t attr;
  void *res;
  VOID_PTR_INT_CAST nRes;

  struct sender_info send_info; // no need to malloc since always 1 instance
  struct receiver_info recv_info;

  /* taking the command line arguments */
  if (argc != 4) {
    fprintf(stderr,
            "Usage: %s <send-to-host> <send-to-port> <receive-from-port>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  char *send_to_host = argv[1];
  char *send_to_port = argv[2];
  char *receive_from_port = argv[3];
  send_to_host = argv[1];
  send_to_port = argv[2];
  receive_from_port = argv[3];

  /* error checkings */
  if (check_args(send_to_host, send_to_port, receive_from_port) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* thread creation attributes */
  s = pthread_attr_init(&attr);
  if (s != 0)
    handle_error_en(s, "pthread_attr_init");
  /* I probably could make a `safe_call` routine/macro which wraps around
   * this is ugly*/

  /* filling the information for the sender and receiver threads */
  send_info.send_to_host = send_to_host;
  send_info.send_to_port = send_to_port;
  recv_info.receive_from_port = receive_from_port;

  /* create those 2 threads */
  s = pthread_create(&send_info.thread_id, &attr,
                     (void *(*)(void *)) & send_thread, &send_info);
  if (s != 0)
    handle_error_en(s, "pthread_create: send_thread");

int send_thread(char *send_to_host, char *send_to_port) {
  /* create a socket */
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }
  s = pthread_create(&recv_info.thread_id, &attr,
                     (void *(*)(void *)) & receive_thread, &recv_info);
  if (s != 0)
    handle_error_en(s, "pthread_create: receive_thread");

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
  s = pthread_attr_destroy(&attr); /* no longer needs attr */
  if (s != 0)
    handle_error_en(s, "pthread_attr_destroy");

  /* send data */
  char *msg = "Hello, world!";
  int numbytes =
      sendto(sockfd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
  if (numbytes < 0) {
    perror("sendto");
    return EXIT_FAILURE;
  }
  /** wait for the threads to finish, join */
  s = pthread_join(send_info.thread_id, &res);
  if (s != 0)
    handle_error_en(s, "pthread_join: send_thread");
  nRes = (VOID_PTR_INT_CAST)res;
  printf("send_thread joined, total messages sent" INT_FMT "\n", nRes);
  
  s = pthread_join(recv_info.thread_id, &res);
  if (s != 0)
    handle_error_en(s, "pthread_join: receive_thread");
  nRes = (VOID_PTR_INT_CAST)res;
  printf("receive_thread joined, total messages received: " INT_FMT "\n", nRes);

  freeaddrinfo(res);
  close(sockfd);
  return EXIT_SUCCESS;
  
  exit(EXIT_SUCCESS);
}


int receive_thread(char *receive_from_port) {
  /* create a socket */
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }
  /* set up the address to receive from */
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  int status = getaddrinfo(NULL, receive_from_port, &hints, &res);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return EXIT_FAILURE;
  }
  /* bind the socket */
  status = bind(sockfd, res->ai_addr, res->ai_addrlen);
  if (status < 0) {
    perror("bind");
    return EXIT_FAILURE;
  }
  /* receive data */
  char buf[100];
  struct sockaddr_storage their_addr;
  socklen_t addr_len = sizeof(their_addr);
  int numbytes = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&their_addr, &addr_len);
  if (numbytes < 0) {
    perror("recvfrom");
    return EXIT_FAILURE;
  }
  buf[numbytes] = '\0';
  printf("received: %s\n", buf);
  freeaddrinfo(res);
  close(sockfd);
  return EXIT_SUCCESS;
}
