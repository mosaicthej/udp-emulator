#include "middleend.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool sameAddr(struct sockaddr * a, struct sockaddr * b){
  char a_host[NI_MAXHOST], a_serv[NI_MAXSERV];
  char b_host[NI_MAXHOST], b_serv[NI_MAXSERV];
  int s;
  s = getnameinfo(a, sizeof(struct sockaddr), 
                  a_host, NI_MAXHOST, 
                  a_serv, NI_MAXSERV, 
                  NI_NUMERICHOST | NI_NUMERICSERV);
  if (s != 0) {
    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
      }
  s = getnameinfo(b, sizeof(struct sockaddr), 
                  b_host, NI_MAXHOST, 
                  b_serv, NI_MAXSERV, 
                  NI_NUMERICHOST | NI_NUMERICSERV);
  if (s != 0) {
    fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
      }
  return (strcmp(a_host, b_host) == 0) && (strcmp(a_serv, b_serv) == 0);
} 

