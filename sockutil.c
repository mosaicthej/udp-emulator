#include "middleend.h"
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool sameAddr(struct sockaddr *a, socklen_t alen, struct sockaddr *b,
              socklen_t blen) {
  if (alen != blen) {
    return false;
  }
  char a_host[NI_MAXHOST], a_serv[NI_MAXSERV];
  char b_host[NI_MAXHOST], b_serv[NI_MAXSERV];
  int s;
  s = getnameinfo(a, alen, a_host, NI_MAXHOST, a_serv, NI_MAXSERV,
                  NI_NUMERICHOST | NI_NUMERICSERV);
  if (s != 0) {
    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }
  s = getnameinfo(b, blen, b_host, NI_MAXHOST, b_serv, NI_MAXSERV,
                  NI_NUMERICHOST | NI_NUMERICSERV);
  if (s != 0) {
    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }
  return (strcmp(a_host, b_host) == 0) && (strcmp(a_serv, b_serv) == 0);
}

/*
 * pickToSend:
 * find the correct addrinfo to use when doing a send between 2 endpoints.
 *
 * @param p1, p2 : the addrinfo structs for the 2 endpoints
 *
 * @param msgAddr, len_msgAddr :
 *   the address and length, decribes the sender of the current message.
 *
 * @param nameTable : the lookup table to use to store the endpoint's info.
 *
 * @return : the correct addrinfo to use when sending the message.
 * */
struct addrinfo *pickToSend(struct addrinfo *p1, struct addrinfo *p2,
                            struct sockaddr *msgAddr, socklen_t len_msgAddr,
                            LookupTable *nameTable) {
  char msgFromHost[NI_MAXHOST], msgFromServ[NI_MAXSERV], p1Host[NI_MAXHOST],
      p2Host[NI_MAXHOST], p1Serv[NI_MAXSERV], p2Serv[NI_MAXSERV];
  EndPointName *left, *right, *dest;
  int s;

#ifdef _COMMENT_
#undef _COMMENT_
#endif
/* the structure of the LookupTable and EndPointName is like following: */
#ifdef _COMMENT_
  typedef struct _endPointInfo {
    char hostName[NI_MAXHOST]; /* Hostname, we only support 1 hostname here */
    char portRcv[NI_MAXSERV];  /*  port used for receiving */
    char portSnd[NI_MAXSERV];  /* port used for sending */
  } EndPointName;              /* this is 1 row on the table */

  typedef struct _lookupTable {
    EndPointName *left;  /* left endpoint */
    EndPointName *right; /* right endpoint */
  } LookupTable;         /* this is the table */
#endif
  /* the passed in msgAddr and len_msgAddr can be used to find:
   * `hostName` and `portSnd` of the sender of the message. (H_x, Ps_x)
   * Using that, look into the table and
   * find the OTHER's `hostName` and `portRcv`
   *
   * Using the 2 passed-in addrinfo, also find their `hostName` and `portRcv`
   *
   * return the one that has a match with the one we want (OTHER)
   * */

  left = nameTable->left;
  right = nameTable->right;

  s = getnameinfo(msgAddr, len_msgAddr, msgFromHost, NI_MAXHOST, msgFromServ,
                  NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
  if (s != 0) {
    fprintf(stderr, "[getnameinfo]: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  s = getnameinfo(p1->ai_addr, p1->ai_addrlen, p1Host, NI_MAXHOST, p1Serv,
                  NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
  if (s != 0) {
    fprintf(stderr, "[getnameinfo]: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  s = getnameinfo(p2->ai_addr, p2->ai_addrlen, p2Host, NI_MAXHOST, p2Serv,
                  NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
  if (s != 0) {
    fprintf(stderr, "[getnameinfo]: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  if (strcmp(left->hostName, msgFromHost) == 0 &&
      strcmp(left->portSnd, msgFromServ) == 0) {
    dest = right;
  } else if (strcmp(right->hostName, msgFromHost) == 0 &&
             strcmp(right->portSnd, msgFromServ) == 0) {
    dest = left;
  } else {
    fprintf(stderr,
            "[pickToSend]: we have a problem here\n"
            "info: msgFromHost: %s, msgFromServ: %s\n"
            "left->hostName: %s, left->portSnd: %s\n"
            "right->hostName: %s, right->portSnd: %s\n",
            msgFromHost, msgFromServ, left->hostName, left->portSnd,
            right->hostName, right->portSnd);
    exit(EXIT_FAILURE);
  }

  /* dest contains the information we want to send to */
  if ((strcmp(dest->hostName, p1Host)==0) && (strcmp(dest->portRcv, p1Serv)==0)) {
    return p1;
  } else if ((strcmp(dest->hostName, p2Host)==0) && (strcmp(dest->portRcv, p2Serv)==0)) {
    return p2;
  } else {
    fprintf(stderr,
            "[pickToSend]: we have a problem here\n"
            "info: dest->hostName: %s, dest->portRcv: %s\n"
            "p1Host: %s, p1Serv: %s\n"
            "p2Host: %s, p2Serv: %s\n",
            dest->hostName, dest->portRcv, p1Host, p1Serv, p2Host, p2Serv);
    exit(EXIT_FAILURE);
  }
}
