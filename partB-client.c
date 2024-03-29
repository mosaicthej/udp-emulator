/* name:  Mark Jia
 * NSID:  mij623
 * stuN:  11271998
 * */

#include <errno.h>
#include <stdbool.h>
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

#define MAX_MSG_SIZE 100

/* arguments to each thread. */
typedef struct sender_info {
  pthread_t thread_id; /* set by pthread_create, parent has access to */
  char *send_to_host;
  char *send_to_port;
  char *rply_to;

  VOID_PTR_INT_CAST nMsgRet;
} Sender_info;

typedef struct receiver_info {
  pthread_t thread_id;
  char *receive_from_port;

  VOID_PTR_INT_CAST nMsgRet;
} Receiver_info;
/* TODO: kill message should also be an argument
add it to sender info and receiver info, also refactor the routines. */

void* send_thread(void *);
void* receive_thread(void *);
/* TODO: Extract the routines as functions,
 * then use thread functions to wrap.
 * So sender and listener can be reused.
 * */

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

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    /* this is ipv4 */
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
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

  Sender_info send_info; /* no need to malloc since always 1 instance */
  Receiver_info recv_info;

  /* taking the command line arguments */
  if (argc != 4) {
    fprintf(stderr,
            "Usage: %s <send-to-host> <send-to-port> <receive-from-port>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  send_to_host = argv[1];
  send_to_port = argv[2];
  receive_from_port = argv[3];

  /* error checkings */
  if (check_args(send_to_host, send_to_port, receive_from_port) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* args has no problemo, will fill in the args */
  send_info.send_to_host = send_to_host;
  send_info.send_to_port = send_to_port;
  send_info.rply_to = receive_from_port; /* need to send it out too. 
  only as initial message */ 
  recv_info.receive_from_port = receive_from_port;

  /* thread creation attributes */
  s = pthread_attr_init(&attr);
  if (s != 0)
    handle_error_en(s, "pthread_attr_init");
  /* I probably could make a `safe_call` routine/macro which wraps around
   * this is ugly*/

  /* create those 2 threads */
  s = pthread_create(&send_info.thread_id, &attr,
                     (void *(*)(void *)) & send_thread, &send_info);
  if (s != 0)
    handle_error_en(s, "pthread_create: send_thread");

  s = pthread_create(&recv_info.thread_id, &attr,
                     (void *(*)(void *)) & receive_thread, &recv_info);
  if (s != 0)
    handle_error_en(s, "pthread_create: receive_thread");
  s = pthread_attr_destroy(&attr); /* no longer needs attr */
  if (s != 0)
    handle_error_en(s, "pthread_attr_destroy");

  /** wait for the threads to finish, join */
  s = pthread_join(send_info.thread_id, &res);
  if (s != 0)
    handle_error_en(s, "pthread_join: send_thread");
  nRes = send_info.nMsgRet;
  printf("send_thread joined, total messages sent " INT_FMT "\n", nRes);

  s = pthread_join(recv_info.thread_id, &res);
  if (s != 0)
    handle_error_en(s, "pthread_join: receive_thread");
  nRes = recv_info.nMsgRet;
  printf("receive_thread joined, total messages received: " INT_FMT "\n", nRes);

  exit(EXIT_SUCCESS);
}

/* sender thread
 * set up connection,
 *  - read input from stdin,
 *  - send it out,
 *  - repeat, until received the kill signal "exit" or EOF
 * return number of messages sent.
 *
 * If error, exit with EXIT_FAILURE.
 * */
void* send_thread(void *arg) {
  /* util */
  VOID_PTR_INT_CAST nMsgSent; /* thread return value, number of messages sent */
  VOID_PTR_INT_CAST* nMsgSentRet; /* return by storing value in passed in arg */
  
  int s;                      /* return val of sys and lib calls */
  void *spt;                  /* return val, but when pointer */
  bool done, hasProblemo;     /* flags */
  /* args */
  Sender_info *send_info;
  char *send_to_host;
  char *send_to_port;
  char *rpy_to;

  /* network stuff */
  int sockfd;
  int numbytes;
  char buf[MAX_MSG_SIZE];
  struct addrinfo hints, /* hints about the type of socket */
      *servinfo,         /* linked list of results */
      *p;                /* to hold the nodes inside linked list */

  /* extracting the arguments */
  if (arg == NULL)
    handle_error("send_thread: arg is NULL");
  send_info = (Sender_info *)arg;
  send_to_host = send_info->send_to_host;
  send_to_port = send_info->send_to_port;
  if (send_to_host == NULL || send_to_port == NULL)
    handle_error("send_thread: send_to_host or send_to_port is NULL");
  nMsgSentRet = &(send_info->nMsgRet);
  rpy_to = send_info->rply_to;
  
  spt = memset(&hints, 0, sizeof(hints));
  if (spt == NULL)
    handle_error("memset in send_thread");

  hints.ai_family = AF_INET;      /* IPv4 */
  hints.ai_socktype = SOCK_DGRAM; /* UDP (datagram) */

  if ((s = getaddrinfo(send_to_host, send_to_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  } /* obtain the addr info */

  /* loop through the result and make a socket */
  done = false;
  hasProblemo = false;
  p = servinfo;
  while(p!=NULL && !done) {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd < 0) {
      perror("socket");
      hasProblemo = true;
    }
    done = !hasProblemo; /* if no problem, done
      otherwise go to next socket until run out */
    hasProblemo = false; /* reset the flag */
    if (!hasProblemo) p = p->ai_next; /* go to next socket if not done */
  }

  if (p == NULL) { /* if no socket is created */
    fprintf(stderr, "send_thread: failed to create socket\n");
    exit(EXIT_FAILURE);
  }

  /* before the main loop. send 1 initial message about
   * which port it is listening on */
  printf("[send_thread]: told the other party my rpy_to is %s", rpy_to);
  numbytes = sendto(sockfd, rpy_to, strlen(rpy_to),0,
                    p->ai_addr, p->ai_addrlen);
  if (numbytes < 0) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }

  nMsgSent = 0;
  done = false;
  while (!done) {
    /* read from stdin */
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
      if (!feof(stdin))
        perror("fgets"); /* some other shits happened in stdin */
      done = true;
      hasProblemo = true;
    }

    if (!hasProblemo) {
      /* send data, if input is good */
      numbytes = sendto(sockfd, buf, strlen(buf), 0, p->ai_addr, p->ai_addrlen);
      if (numbytes < 0) {
        perror("sendto");
        done = true;
        hasProblemo = true;
        exit(EXIT_FAILURE);
      }
      nMsgSent++;
    }
    done = strcmp(buf, "exit\n") == 0 || strcmp(buf, "exit") == 0; /* kill sig*/
  }

  /* done main loop */
  freeaddrinfo(servinfo);
  printf("send_thread: sent " INT_FMT " messages\n", nMsgSent);
  close(sockfd);

  *nMsgSentRet = nMsgSent;
  return (void*) nMsgSent;
}

/* receiver thread
 * set up connection,
 * - receive data,
 *  - print it out,
 *  - repeat, until received the kill signal "exit" or EOF
 *  return number of messages received.
 * */
void* receive_thread(void *arg) {
  /* util */
  VOID_PTR_INT_CAST nMsgRecv; /* thread return value, num msg received */
  VOID_PTR_INT_CAST* nMsgRecvRet;     /* bruh even clang did not complain */
  int s;                      /* return val of sys and lib calls */
  void *spt;                  /* return val, but when pointer */
  bool done, hasproblemo;     /* flags */
  /* args */
  Receiver_info *recv_info;
  char *receive_from_port;
  /* network */
  int sockfd;
  int numBytes;
  char buf[MAX_MSG_SIZE];
  struct addrinfo hints, *servinfo, *p;
  socklen_t addr_len;
  struct sockaddr_storage their_addr;
  char their_addr_st[INET_ADDRSTRLEN];

  spt = memset(&hints, 0, sizeof(hints));
  if (spt == NULL)
    handle_error("memset in receive_thread");
  hints.ai_family = AF_INET;      /* IPv4 */
  hints.ai_socktype = SOCK_DGRAM; /* UDP (datagram) */
  hints.ai_flags = AI_PASSIVE;    /* use my IP */

  if (arg == NULL)
    handle_error("receive_thread: arg is NULL");
  recv_info = (Receiver_info *)arg;
  receive_from_port = recv_info->receive_from_port;
  nMsgRecvRet = &(recv_info->nMsgRet);

  if ((s = getaddrinfo(NULL, receive_from_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  } /* obtain the addr info */

  /* find socket to bind */
  done = false;
  hasproblemo = false;
  p = servinfo;
  while(p!=NULL && !done){    
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    hasproblemo = false;
    if (sockfd < 0) {
      perror("receive_thread: socket");
      hasproblemo = true;
    }
    done = !hasproblemo; /* if no problem, done
    otherwise go to next socket until run out */
    if (!hasproblemo) {  /* if no problem, bind it */
      if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
        close(sockfd);
        perror("receiver thread: bind");
        hasproblemo = true;
      }
    }
    done = done && !hasproblemo; /* both action needs to be successful */
    if(!done) p = p->ai_next; /* go to next socket if not done */
  }

  if (p == NULL)
    handle_error("receive_thread: failed to bind/create socket");

  freeaddrinfo(servinfo); /* no longer needs servinfo */

  printf("listener: waiting to recvfrom...\n"); /* DEBUG message */
  /* main loop to receive data */
  addr_len = sizeof(their_addr);
  done = false;
  hasproblemo = false;
  nMsgRecv = 0;
  while (!done) {
    if ((numBytes = recvfrom(sockfd, buf, MAX_MSG_SIZE - 1, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) < 0) {
      perror("receive_thread: recvfrom");
      hasproblemo = true;
    }
    /* got a message! (this is blocking via recvfrom) */

    printf("listener: got packet from %s\n",
           inet_ntop(their_addr.ss_family,
                     get_in_addr((struct sockaddr *)&their_addr), their_addr_st,
                     sizeof their_addr_st));
    printf("listener: packet is %d bytes long\n", numBytes);
    buf[numBytes] = '\0'; /* swap the end with \0 */
    printf("listener: packet contains \"%s\"\n", buf);
    nMsgRecv++;
    done = strcmp(buf, "exit\n") == 0 || strcmp(buf, "exit") == 0; /* kill sig*/
  }
  printf("receive_thread: done, received " INT_FMT " messages\n", nMsgRecv);
  close(sockfd);

  *nMsgRecvRet=nMsgRecv;
  return (void*) nMsgRecv;
}
/* TODO: printouts should include thread number as well. */
