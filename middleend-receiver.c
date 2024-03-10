#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <queue.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


#include "middleend.h"
#include "conn.h"
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

  LookupTable *nameTbl;

  /* network */
  int sockfd;
  int numBytes;
  struct sockaddr * from_addr1, * from_addr2, * tmp_addr;
  socklen_t len_addr1, len_addr2, len_tmp;
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
  nameTbl = recv_info->nameTbl;
  
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
    if (!dropped &&
       !(done && sameAddr((struct sockaddr *)&their_addr, addr_len, 
                          from_addr1, len_addr1)) &&
       !(done2 && sameAddr((struct sockaddr *)&their_addr, addr_len,
                           from_addr2, len_addr2))) {
    /* not dropped, and not from a closed buffer */
      tmp_addr = (struct sockaddr *)&their_addr;
      len_tmp = addr_len;
      /* got a message! (this is blocking via recvfrom) */
      if (nMsgRecv1 == 0) {
      /* case: 1st message! set from_addr1 to the sender */
        from_addr1 = tmp_addr;
        len_addr1 = len_tmp;
        nMsgRecv1++;
      /* this message also contains `P_rx` reply port for this endpoint
       * add this information to the table */
        do_getnameinfo(tmp_addr, addr_len, hbuf, sbuf);
        strncpy(nameTbl->left->portSnd, sbuf, NI_MAXSERV);
      } else if (nMsgRecv2 == 0 && /* not the 1st. one, and haven't seen 2nd */
          !sameAddr(tmp_addr, len_tmp, from_addr1, len_addr1) ) {
        from_addr2 = tmp_addr; /* case: 1st message from the other end */
        len_addr2 = len_tmp;
        nMsgRecv2++;
        do_getnameinfo(tmp_addr, addr_len, hbuf, sbuf);
        strncpy(nameTbl->right->portSnd, sbuf, NI_MAXSERV);
      } else if (!sameAddr(tmp_addr, len_tmp, from_addr1, len_addr1) &&
                !sameAddr(tmp_addr, len_tmp, from_addr2, len_addr2)){
        fprintf(stderr, "receive_thread: we have a problem here\n");
        exit(EXIT_FAILURE); /* neither is correct */
      } else if (sameAddr(tmp_addr, len_tmp, from_addr1, len_addr1)) {
        nMsgRecv1++;
      } else if (sameAddr(tmp_addr, len_tmp, from_addr2, len_addr2)) {
        nMsgRecv2++;
      } else {
        fprintf(stderr, "receive_thread: we have a problem here\n");
        exit(EXIT_FAILURE); /* wtf */
      }

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
      Qmsg->fromAddr = (struct sockaddr *)&their_addr;
      Qmsg->fromAddrLen = addr_len;
      
      pthread_mutex_lock(QLock);
      QEnqueue(messagesQ, Qmsg);
      pthread_mutex_unlock(QLock);
      
      if(sameAddr(Qmsg->fromAddr, Qmsg->fromAddrLen, from_addr1, len_addr1)) 
        do_testkill(rcvBuf, killMsg, done);
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
