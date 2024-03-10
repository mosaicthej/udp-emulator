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
#include "conn.h" /* for the macro functions */

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

#define PORTMAX 65535
#define PORTMIN 1024

#define MAX_MSG_SIZE 256
#define MAX_ADDR_LEN 128
#define MAX_PORT_LEN 32

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
  VOID_PTR_INT_CAST retVal;
} Sender_info;

typedef struct receiver_info {
  pthread_t thread_id;
  char *receive_from_port;

  float drop_prob; /* drop probility */

  QUEUE *messagesQ;       /* list of messages */
  pthread_mutex_t *QLock; /* lock for the list */
  char *killMsg;          /* message to end the conversation */
  VOID_PTR_INT_CAST nRecv1;
  VOID_PTR_INT_CAST nRecv2; /* statistics */
  VOID_PTR_INT_CAST retVal;
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
  char e1n[MAX_ADDR_LEN], e2n[MAX_ADDR_LEN];
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
  char* receive_from_port;

  /* theirs addr */
  char endpoint1_host[MAX_ADDR_LEN], endpoint2_host[MAX_ADDR_LEN];
  char send_to_port1[MAX_PORT_LEN], send_to_port2[MAX_PORT_LEN];

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
  send_info.retVal = 0;

  recv_info.receive_from_port = receive_from_port;
  recv_info.messagesQ = messagesQ;
  recv_info.QLock = &QLock;
  recv_info.killMsg = "exit";
  recv_info.drop_prob = drop_prob;
  recv_info.retVal = 0;

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
  /* void *spt; */                       /* return val, but when pointer */
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
  char hbuf1[NI_MAXHOST], sbuf1[NI_MAXSERV];
  char hbuf2[NI_MAXHOST], sbuf2[NI_MAXSERV];
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
  ret = &(send_info->retVal);

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
  do_sender_findHS(p1, hbuf1, sbuf1);
  printf("connected to dest 1, \thost: %s, \tserv: %s\n", hbuf1, sbuf1);

  do_setup_hints(hints2, 0, sizeof(hints2),
                 "setup_hints in [send_thread, hints 2]: memset failed");
  do_getaddrinfo(s, send_to_host2, send_to_port2, hints2, servinfo2);
  do_socket_walk(p2, servinfo2, sockfd2, "[send_thread, socket 2]");
  do_sender_findHS(p2, hbuf2, sbuf2);
  printf("connected to dest 2, \thost: %s, \tserv: %s\n", hbuf2, sbuf2);
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
      sleep(delay * 2);
      pthread_mutex_lock(QLock);
      hasMsg = (Qmsg = QDequeue(messagesQ)) != NULL;
      pthread_mutex_unlock(QLock);
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
      if(hasProblemo) done = true;
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
  hasMsg = false; 
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
  int s;                                  /* return val of sys and lib calls */
  /* void *spt; */                        /* return val, but when pointer */
  bool done, done2, hasproblemo, dropped; /* flags */
  /* args */
  Receiver_info *recv_info;
  char *receive_from_port;
  float drop_prob, rnd;   /* drop probility */
  QUEUE *messagesQ;       /* list of messages */
  pthread_mutex_t *QLock; /* lock for the list */
  char *killMsg;

  /* network */
  int sockfd;
  int numBytes;
  in_addr_t from_addr1, from_addr2, tmp_addr;
#ifdef MALLOCMSG
  ChannelMsg *Qmsg; /* message in buffer from malloc. */
  char rcvBuf[MAX_MSG_SIZE];
  char *buf; /* also malloced */

#else
  char buf[MAX_MSG_SIZE];
#endif
  struct addrinfo hints, *servinfo, *p;
  socklen_t addr_len;
  struct sockaddr_storage their_addr;
  char their_addr_st[INET_ADDRSTRLEN];

  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

  if (arg == NULL)
    handle_error("receive_thread: arg is NULL");
  recv_info = (Receiver_info *)arg;
  receive_from_port = recv_info->receive_from_port;
  drop_prob = recv_info->drop_prob;
  messagesQ = recv_info->messagesQ;
  QLock = recv_info->QLock;
  killMsg = recv_info->killMsg;
  nMsgRecvRet1 = &(recv_info->nRecv1);
  nMsgRecvRet2 = &(recv_info->nRecv2);
  ret = &(recv_info->retVal);
  
#ifndef CONNMACRO
  handle_error("CONNMACRO is not defined");
#else
  do_setup_hints(hints, 0, sizeof(hints),
                 "setup_hints in [receive_thread, hints]: memset failed");
  hints.ai_flags = AI_PASSIVE; /* use my IP */
  do_getaddrinfo(s, NULL, receive_from_port, hints, servinfo);
#endif
  /* find socket to bind */
  /* could have used a macro..., which can further extract by
   * include body part (can extracting from ) do_socket_walk above
   * If only I had more time */
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

  printf("listener: waiting to recvfrom...\n"); /* DEBUG message */
  /* main loop to receive data */
  addr_len = sizeof(their_addr);
  done = false; done2 = false;
  hasproblemo = false;
  nMsgRecv1 = 0;
  nMsgRecv2 = 0;
  while (!done && !done2){
    if ((numBytes = recvfrom(sockfd, rcvBuf, MAX_MSG_SIZE - 1, 0,
                             (struct sockaddr *)&their_addr, &addr_len)) < 0) {
      perror("receive_thread: recvfrom");
      hasproblemo = true;
    }
    if(getnameinfo((struct sockaddr *)&their_addr, addr_len, 
                  hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                  NI_NUMERICHOST | NI_NUMERICSERV) == 0)
        printf("[receive_thread]: got message:\n"
        "host: %s, \tserv: %s\n", hbuf, sbuf);
    if (hasproblemo) {
      done = true;
      continue;
    }
    gen_rand(rnd);
    dropped = rnd < drop_prob;
    if (!dropped) {
      /* got a message! (this is blocking via recvfrom) */
      do_saddrsto_to_sin_addr(their_addr, tmp_addr);
      if (nMsgRecv1 ==
          0) { /* case: 1st message! set from_addr1 to the sender */
        from_addr1 = tmp_addr;
        nMsgRecv1++;
      } else if (nMsgRecv2 == 0 && tmp_addr != from_addr1) {
        from_addr2 = tmp_addr; /* case: 1st message from the other end */
        nMsgRecv2++;
      } else if (tmp_addr != from_addr1 && tmp_addr != from_addr2) {
        fprintf(stderr, "receive_thread: we have a problem here\n");
        exit(EXIT_FAILURE); /* neither is correct */
      } else if (tmp_addr == from_addr1) {
        nMsgRecv1++;
      } else if (tmp_addr == from_addr2) {
        nMsgRecv2++;
      } else {
        fprintf(stderr, "receive_thread: we have a problem here\n");
        exit(EXIT_FAILURE); /* wtf */
      }

      printf("listener: got packet from %s\n",
             inet_ntop(their_addr.ss_family,
                       get_in_addr((struct sockaddr *)&their_addr),
                       their_addr_st, sizeof their_addr_st));
      printf("listener: sender id for this packet is %d\n",
             (tmp_addr == from_addr1) ? 1 : 2);
      printf("listener: packet is %d bytes long\n", numBytes);
      rcvBuf[numBytes] = '\0'; /* swap the end with \0 */
      printf("listener: packet contains \"%s\"\n", rcvBuf);

      buf = malloc(numBytes + 1);
      if (buf == NULL)
        handle_error("receive_thread: malloc failed");
      strcpy(buf, rcvBuf);
      Qmsg = malloc(sizeof(ChannelMsg));
      if (Qmsg == NULL)
        handle_error("receive_thread: malloc failed");
      Qmsg->msg = buf;
      Qmsg->from = tmp_addr;

      pthread_mutex_lock(QLock);
      QEnqueue(messagesQ, Qmsg);
      pthread_mutex_unlock(QLock);
      
      if(Qmsg->from == from_addr1) do_testkill(rcvBuf, killMsg, done);
      else do_testkill(rcvBuf, killMsg, done2);
    } else printf("listener: packet dropped :P\n"); /* dropped */ 
  }
  /* done main loop */
  do_done_cleanup(servinfo, sockfd);
  printf("listener thread: done, received" INT_FMT "messages from sender 1\n"
         "and received " INT_FMT " messages from sender 2\n",
         nMsgRecv1, nMsgRecv2);

  *nMsgRecvRet1 = nMsgRecv1;
  *nMsgRecvRet2 = nMsgRecv2;
  *ret = nMsgRecv1 + nMsgRecv2;
  return (void *)(ret);
}
/* TODO: printouts should include thread number as well. */
