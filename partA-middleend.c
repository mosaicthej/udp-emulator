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
#else
#define VOID_PTR_INT_CAST int /* default int */
#define INT_FMT "%d"
#endif

/* getaddrinfo */
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>

/* list library */
#include <queue.h>

#define MALLOCMSG /* messages are malloced,                                    \
otherwise, I'd got static data structure to hold them                          \
note that, when free,                                                          \
need to free both ChannelMsg and the msg inside it.                            \
*/

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

#define CONNMACRO

#define do_setup_hints(s, c, n, e)                                             \
  do {                                                                         \
    if (memset(&(s), (c), (n)) == NULL)                                        \
      handle_error((e));                                                       \
    s.ai_family = AF_INET;      /* IPv4 */                                     \
    s.ai_socktype = SOCK_DGRAM; /* UDP (datagram) */                           \
  } while (0)

/* temp, sendName, sendPort, hints, servinfo */
#define do_getaddrinfo(t, n, p, h, s)                                          \
  do {                                                                         \
    if (((t) = getaddrinfo((n), (p), &(h), &(s))) != 0) {                      \
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(t));                   \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/* p, servinfo, sockfd */
#define do_socket_walk(p, s, f, e)                                             \
  do {                                                                         \
    for ((p) = (s); (p) != NULL; (p) = (p)->ai_next) {                         \
      (f) = socket((p)->ai_family, (p)->ai_socktype, (p)->ai_protocol);        \
      if ((f) < 0) {                                                           \
        perror("socket");                                                      \
        continue;                                                              \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
                                                                               \
    if ((p) == NULL) { /* if no socket is created */                           \
      fprintf(stderr, "%s: failed to create socket\n", (e));                   \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/* from p to sendto addr */
#define do_p_to_sin_addr(p, a)                                                 \
  do {                                                                         \
    (a) =                                                                      \
        (((struct sockaddr_in *)(((p)->ai_addr)->sa_data))->sin_addr).s_addr;  \
  } while (0)

/* given the addr part of ChannelMsg and 2 addrinfo,
 * find one that the message should go.
 * (if from==p_i->sin_addr, then return p_j)
 * */
#define do_find_msgDest_addrinfo(f, p, q, r)                                   \
  do {                                                                         \
    if ((f) == (p))                                                            \
      (r) = (q);                                                               \
    else                                                                       \
      (r) = (p);                                                               \
  } while (0)

#define do_sendto(sock, msg, p, done)                                          \
  do {                                                                         \
    numbytes =                                                                 \
        sendto((sock), (msg), strlen(msg), 0, (p)->ai_addr, (p)->ai_addrlen);  \
    if (numbytes < 0) {                                                        \
      perror("sendto");                                                        \
      hasProblemo = true;                                                      \
      (done) = true;                                                           \
    }                                                                          \
                                                                               \
  } while (0)

#define do_testkill(msg, kill, done)                                           \
  do {                                                                         \
    if (!((done) = (strcmp((msg), (kill)) == 0))) /* if not same string */     \
      (done) = ((strncmp((msg), (kill), strlen((kill))) == 0) &&               \
                (strlen((msg)) - strlen((msg)) == 1) &&                        \
                ((msg)[strlen((kill))] == '\n'));                              \
  } while (0)
#define do_free_msg(m)                                                         \
  do {                                                                         \
    free((m)->msg);                                                            \
    free((m));                                                                 \
  } while (0)

#define do_done_cleanup(servinfo, sockfd)                                      \
  do {                                                                         \
    freeaddrinfo((servinfo));                                                  \
    close((sockfd));                                                           \
  } while (0)

#define do_done_send_print(nMsg, who)                                          \
  do {                                                                         \
    printf("send_thread: sent " INT_FMT "messages to %s \n", (nMsg), (who));   \
    printf("send_thread: stream to %s is done, socket has closed\n", (who));   \
  } while (0)

#define PORTMAX 65535
#define PORTMIN 1024

#define MAX_MSG_SIZE 100

#define MAX_DELAY 10 /* seconds */

/* arguments to each thread. */
typedef struct sender_info {
  pthread_t thread_id; /* set by pthread_create, parent has access to */
  char *send_to_host1;
  char *send_to_port1;

  char *send_to_host2;
  char *send_to_port2;

  int propgDelay; /* how often to check the queue */

  QUEUE *messagesQ;       /* list of messages */
  pthread_mutex_t *QLock; /* lock for the list */
  char *killMsg;          /* message to end the conversation */

  VOID_PTR_INT_CAST nSent1to2;
  VOID_PTR_INT_CAST nSent2to1; /* statistics */
} Sender_info;

typedef struct receiver_info {
  pthread_t thread_id;
  char *receive_from_port;

  QUEUE *messagesQ;       /* list of messages */
  pthread_mutex_t *QLock; /* lock for the list */
  char *killMsg;          /* message to end the conversation */
  VOID_PTR_INT_CAST nRecv1;
  VOID_PTR_INT_CAST nRecv2; /* statistics */
} Receiver_info;
/* TODO: kill message should also be an argument
add it to sender info and receiver info, also refactor the routines. */

typedef struct _channelMsg {
  in_addr_t from;
  char *msg;
} ChannelMsg;

void *send_thread(void *);
void *receive_thread(void *);
/* TODO: Extract the routines as functions,
 * then use thread functions to wrap.
 * So sender and listener can be reused.
 * */

/* validate port numbers */
int check_args(char *p, char *d, char *listen_on, char *e1, char *e2) {
  int port;
  float prob;
  int delay;
  char *e1n, *e2n;
  int e1p, e2p;
  int s;

  port = atoi(listen_on);
  prob = atof(p);
  delay = atoi(d);

  if (port < PORTMIN || port > PORTMAX) {
    fprintf(stderr, "port must between %d and %d\n", PORTMIN, PORTMAX);

    return EXIT_FAILURE;
  }
  if (prob < 0.0 || prob > 1.0) {
    fprintf(stderr, "probility must between 0.0 and 1.0\n");
    return EXIT_FAILURE;
  }
  if (delay < 0 || delay > MAX_DELAY) {
    fprintf(stderr, "delay must between 0 and %d (s)\n", MAX_DELAY);
    return EXIT_FAILURE;
  }
  if (e1 == NULL || e2 == NULL) {
    fprintf(stderr, "endpoint-1 and endpoint-2 cannot be NULL\n");
    return EXIT_FAILURE;
  }

  s = sscanf(e1, "%[^:]:%d", e1n, &e1p);
  if (s != 2) {
    fprintf(stderr, "endpoint-1 must be in format <hostname>:<port>\n");
    return EXIT_FAILURE;
  }
  s = sscanf(e2, "%[^:]:%d", e2n, &e2p);
  if (s != 2) {
    fprintf(stderr, "endpoint-2 must be in format <hostname>:<port>\n");
    return EXIT_FAILURE;
  }

  if (e1p < PORTMIN || e1p > PORTMAX || e2p < PORTMIN || e2p > PORTMAX) {
    fprintf(stderr, "endpoint port must between %d and %d\n", PORTMIN, PORTMAX);
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
 * Program: middleend
 *   2 threads, 1 for sending, 1 for receiving.
 *  using posix threads to do threading.
 *
 *  arguments from command line:
 *  - `p`  : drop probility, 0.0 <= p <= 1.0
 *  - `d`  : delay, 0 <= d <= 1000 (ms)
 *  - `listen-on-port` : 1024 <= port <= 65535
 *  - `endpoint-1` : <hostname>:<port>
 *  - `endpoint-2` : <hostname>:<port>
 * */

int main(int argc, char *argv[]) {
  int s; /* keep track status of system calls */

  /* cmd arguments */
  float drop_prob; /* drop probility */
  int delay;       /* delay in ms */
  char *receive_from_port;
  char *endpoint1, *endpoint2; /* network locs */

  /* theirs addr */
  char *endpoint1_host, *endpoint2_host;
  char *send_to_port1, *send_to_port2;

  /* pthread related things */
  pthread_attr_t attr;
  void *res;
  VOID_PTR_INT_CAST nRes, nRes2;

  Sender_info send_info; /* no need to malloc since always 1 instance */
  Receiver_info recv_info;

  QUEUE *messagesQ; /* QUEUE of messages */

  /* taking the command line arguments */
  if (argc != 6) {
    fprintf(stderr,
            "usage: %s <drop-prob> <delay> <listen-on-port> <endpoint-1> "
            "<endpoint-2>\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  /* error checkings */
  if (check_args(argv[1], argv[2], argv[3], argv[4], argv[5]) != EXIT_SUCCESS)
    return EXIT_FAILURE;

  /* args has no problemo, will fill in the args */
  drop_prob = atof(argv[1]);
  delay = atoi(argv[2]);
  receive_from_port = argv[3];
  s = sscanf(argv[4], "%[^:]:%s", endpoint1_host, send_to_port1);
  if (s != 2) {
    fprintf(stderr, "endpoint-1 must be in format <hostname>:<port>\n");
    exit(EXIT_FAILURE);
  }
  s = sscanf(argv[5], "%[^:]:%s", endpoint2_host, send_to_port2);
  if (s != 2) {
    fprintf(stderr, "endpoint-2 must be in format <hostname>:<port>\n");
    exit(EXIT_FAILURE);
  }

  messagesQ = QueueCreate();
  if (messagesQ == NULL) {
    fprintf(stderr, "QueueCreate failed\n");
    exit(EXIT_FAILURE);
  }
  static pthread_mutex_t QLock = PTHREAD_MUTEX_INITIALIZER;
  /* simple static activate. This is NOT mixed decl and definition,
   * it is the macro way specified by POSIX to init a lock */

  send_info.send_to_host1 = endpoint1_host;
  send_info.send_to_port1 = send_to_port1;
  send_info.send_to_host2 = endpoint2_host;
  send_info.send_to_port2 = send_to_port2;
  send_info.propgDelay = delay;
  send_info.messagesQ = messagesQ;
  send_info.QLock = &QLock;
  send_info.killMsg = "exit";

  recv_info.receive_from_port = receive_from_port;
  recv_info.messagesQ = messagesQ;
  recv_info.QLock = &QLock;
  recv_info.killMsg = "exit";

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
  nRes = send_info.nSent1to2;
  nRes2 = send_info.nSent2to1;
  printf("send_thread joined, total messages sent from 1 to 2: " INT_FMT
         " and total messages sent from 2 to 1: " INT_FMT "\n",
         nRes, nRes2);

  s = pthread_join(recv_info.thread_id, &res);
  if (s != 0)
    handle_error_en(s, "pthread_join: receive_thread");
  nRes = recv_info.nRecv1;
  nRes2 = recv_info.nRecv2;
  printf("receive_thread joined, total messages received from 1: " INT_FMT
         " and total messages received from 2: " INT_FMT "\n",
         nRes, nRes2);

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
void *send_thread(void *arg) {
  /* util */
  /* thread return value, number of messages sent */
  VOID_PTR_INT_CAST nMsgSent1;
  VOID_PTR_INT_CAST nMsgSent2;
  VOID_PTR_INT_CAST *nMsgSentRet1; /* forced by gcc to return (void *) */
  VOID_PTR_INT_CAST *nMsgSentRet2;
  VOID_PTR_INT_CAST *ret;                /* return value */
  int s;                                 /* return val of sys and lib calls */
  void *spt;                             /* return val, but when pointer */
  bool done, done2, hasProblemo, hasMsg; /* flags */
  /* args */
  Sender_info *send_info;
  char *send_to_host1, *send_to_host2;
  char *send_to_port1, *send_to_port2;
  int delay;              /* delay in seconds */
  QUEUE *messagesQ;       /* list of messages */
  pthread_mutex_t *QLock; /* lock for the list */
  char *kill;

  /* network stuff */
  int sockfd1, sockfd2;
  in_addr_t to_addr1, to_addr2, msg_from, msg_to, tmp_addr;
  /* 2 addresses */

  int numbytes;
#ifdef MALLOCMSG
  ChannelMsg *Qmsg; /* message in buffer from malloc. */
#else
  char buf[MAX_MSG_SIZE];
#endif
  struct addrinfo hints1, hints2, /* hints about the type of socket */
      *servinfo1, *servinfo2,     /* linked list of results */
      *p1, *p2, *psend;           /* to hold the nodes inside linked list */

  /* extracting the arguments */
  if (arg == NULL)
    handle_error("send_thread: arg is NULL");
  send_info = (Sender_info *)arg;

  send_to_host1 = send_info->send_to_host1;
  send_to_port1 = send_info->send_to_port1;
  send_to_host2 = send_info->send_to_host2;
  send_to_port2 = send_info->send_to_port2;
  delay = send_info->propgDelay;
  messagesQ = send_info->messagesQ;
  QLock = send_info->QLock;
  kill = send_info->killMsg;

  if (send_to_host1 == NULL || send_to_port1 == NULL || send_to_host2 == NULL ||
      send_to_port2 == NULL)
    handle_error("send_thread: send_to_host1, send_to_port1, send_to_host2, "
                 "send_to_port2 cannot be NULL");
  nMsgSentRet1 = &(send_info->nSent1to2); /* return value */
  nMsgSentRet2 = &(send_info->nSent2to1); /* return value */
/* set up the hints */
#ifndef CONNMACRO
  handle_error("CONNMACRO is not defined");
#else
  do_setup_hints(hints1, 0, sizeof(hints1),
                 "setup_hints in [send_thread, hints 1]: memset failed");
  do_getaddrinfo(s, send_to_host1, send_to_port1, hints1, servinfo1);
  do_socket_walk(p1, servinfo1, sockfd1, "[send_thread, socket 1]");

  do_setup_hints(hints2, 0, sizeof(hints2),
                 "setup_hints in [send_thread, hints 2]: memset failed");
  do_getaddrinfo(s, send_to_host2, send_to_port2, hints2, servinfo2);
  do_socket_walk(p2, servinfo2, sockfd2, "[send_thread, socket 2]");
#endif

  nMsgSent1 = 0;
  nMsgSent2 = 0;
  done = false;
  hasMsg = false;
  while (!done && !done2) {
    /* sleep for 2xdelay, check the queue
     * if the result of dequeue is NULL, then sleep again.
     * Otherwise, send the message out, and free it.
     * */
    while (!hasMsg) {
      sleep(send_info->propgDelay * 2);
      pthread_mutex_lock(QLock);
      hasMsg = (Qmsg = QDequeue(messagesQ)) != NULL;
    }
    /* we have a message here (on Qmsg) */
    /* need to find which to go to */
    do_p_to_sin_addr(p1, to_addr1);
    do_p_to_sin_addr(p2, to_addr2);
    msg_from = Qmsg->from;
    if (msg_from == to_addr1) {
      psend = p2;
      msg_to = to_addr2;
    } else {
      psend = p2;
      msg_to = to_addr1;
    }
    do_p_to_sin_addr(psend, tmp_addr);
    if (tmp_addr != msg_to) {
      fprintf(stderr, "addr_calc: we have a problem here\n");
      exit(EXIT_FAILURE);
    }
    /* if it's 1 -> 2 */
#ifndef CONNMACRO
    handle_error("CONNMACRO is not defined");
#else
    if ((msg_to == to_addr1) && (!done)) {
      do_sendto(sockfd1, Qmsg->msg, psend, done);
      do_testkill(Qmsg->msg, kill, done);
      do_free_msg(Qmsg);
      nMsgSent1++;
      if (done) {
        do_done_cleanup(servinfo1, sockfd1);
        do_done_send_print(nMsgSent1, "1");
      }
    } else if ((msg_to == to_addr2) && (!done2)) {
      do_sendto(sockfd2, Qmsg->msg, psend, done2);
      do_testkill(Qmsg->msg, kill, done2);
      nMsgSent2++;
      if (done2) {
        do_done_cleanup(servinfo2, sockfd2);
        do_done_send_print(nMsgSent2, "2");
      }
    } else {
      fprintf(stderr,
              "send_thread: we have a problem here.\n"
              "info: to_addr1: %s\t to_addr2: %s\n"
              "\t stream to 1 closed: %s\n"
              "\t stream to 2 closed: %s\n",
              (msg_to == to_addr1) ? "yes" : "no",
              (msg_to == to_addr2) ? "yes" : "no", (done) ? "yes" : "no",
              (done2) ? "yes" : "no");
      exit(EXIT_FAILURE);
    }
#endif
  }
  /* done main loop */
  *nMsgSentRet1 = nMsgSent1;
  *nMsgSentRet2 = nMsgSent2;
  *ret = nMsgSent1 + nMsgSent2;
  return (void *)(ret);
  /* thread ended */
}

/* receiver thread
 * set up connection,
 * - receive data,
 *  - print it out,
 *  - repeat, until received the kill signal "exit" or EOF
 *  return number of messages received.
 * */
void *receive_thread(void *arg) {
  /* util */
  /* thread return value, num msg received */
  VOID_PTR_INT_CAST nMsgRecv1, nMsgRecv2;
  /* forced by gcc to return (void *) */
  VOID_PTR_INT_CAST *nMsgRecvRet1, *nMsgRecvRet2, *ret;
  int s;                  /* return val of sys and lib calls */
  void *spt;              /* return val, but when pointer */
  bool done, hasproblemo; /* flags */
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

  if ((s = getaddrinfo(NULL, receive_from_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  } /* obtain the addr info */

  /* find socket to bind */
  done = false;
  hasproblemo = false;
  p = servinfo;
  while (p != NULL && !done) {
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
    if (!done)
      p = p->ai_next; /* go to next socket if not done */
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
  return nMsgRecv;
}
/* TODO: printouts should include thread number as well. */
