/* name:  Mark Jia
 * NSID:  mij623
 * stuN:  11271998
 * */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>

/* list library */
#include <queue.h>
#include "conn.h" /* for the macro functions */
#include "middleend.h"

#define MALLOCMSG /* messages are malloced,                                    \
otherwise, I'd got static data structure to hold them                          \
note that, when free,                                                          \
need to free both ChannelMsg and the msg inside it.                            \
*/

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

  EndPointName left, right;
  LookupTable nameTbl;

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

  /* initialize the `Hx` and `P_rx` entry for each endpoint */
  strncpy(left.hostName, endpoint1_host, NI_MAXHOST);
  strncpy(left.portRcv, send_to_port1, NI_MAXSERV);
  memset(left.portSnd, 0, NI_MAXSERV); /* not yet completed  */
  
  strncpy(right.hostName, endpoint2_host, NI_MAXHOST);
  strncpy(right.portRcv, send_to_port2, NI_MAXSERV);
  memset(right.portSnd, 0, NI_MAXSERV); 
  
  nameTbl.left=&left; nameTbl.right=&right; /* partial lookup table */

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
  send_info.nameTbl = &nameTbl;

  recv_info.receive_from_port = receive_from_port;
  recv_info.messagesQ = messagesQ;
  recv_info.QLock = &QLock;
  recv_info.killMsg = "exit";
  recv_info.drop_prob = drop_prob;
  recv_info.retVal = 0;
  recv_info.nameTbl = &nameTbl;

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

/* TODO: printouts should include thread number as well. */

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


