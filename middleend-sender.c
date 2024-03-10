#include "conn.h"
#include "middleend.h"
#include <errno.h>
#include <pthread.h>
#include <queue.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
  LookupTable *nameTbl;

  /* network stuff */
  int sockfd1, sockfd2;
  struct sockaddr *to_addr1, *to_addr2, *msg_addr, *tmp_addr;
  socklen_t len_addr1, len_addr2, len_msgAddr, len_tmp;
  char hbuf1[NI_MAXHOST], sbuf1[NI_MAXSERV];
  char hbuf2[NI_MAXHOST], sbuf2[NI_MAXSERV];
  /* 2 addresses */

  int numbytes;
#ifdef MALLOCMSG
  ChannelMsg *Qmsg; /* message in buffer from malloc. */
  char *msgContent;
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
    to_addr1 = p1->ai_addr;
    len_addr1 = p1->ai_addrlen;
    to_addr2 = p2->ai_addr;
    len_addr2 = p2->ai_addrlen;

    /* this sender thread is responsible for,
     * - taking the message_from address, then
     * - find the correct `p` to send the message to.
     * - send it.
     * */
    msg_addr = Qmsg->fromAddr;
    len_msgAddr = Qmsg->fromAddrLen;
    msgContent = Qmsg->msg;
    psend = pickToSend(p1, p2, msg_addr, len_msgAddr);
    /* psend is the correct p to send message to */
#ifndef CONNMACRO
    handle_error("CONNMACRO is not defined");
#else
    if (sameAddr(psend->ai_addr, psend->ai_addrlen, p1->ai_addr,
                 p1->ai_addrlen) &&
        (!done)) {
      do_sendto(sockfd1, msgContent, psend, done);
      if (hasProblemo)
        done = true;
      do_testkill(msgContent, kill, done);
      do_free_msg(Qmsg);
      nMsgSent1++;
      if (done) {
        do_done_cleanup(servinfo1, sockfd1);
        do_done_send_print(nMsgSent1, "1");
      }
    } else if (sameAddr(psend->ai_addr, psend->ai_addrlen, p2->ai_addr,
                        p2->ai_addrlen) &&
               (!done2)) {
      do_sendto(sockfd2, msgContent, psend, done2);
      if (hasProblemo)
        done2 = true;
      do_testkill(msgContent, kill, done2);
      do_free_msg(Qmsg);
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
                (sameAddr(psend->ai_addr, psend->ai_addrlen, p1->ai_addr,
                  p1->ai_addrlen)) ? "yes" : "no",
                (sameAddr(psend->ai_addr, psend->ai_addrlen, p2->ai_addr,
                  p2->ai_addrlen)) ? "yes" : "no", 
                (done) ? "yes" : "no",
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
