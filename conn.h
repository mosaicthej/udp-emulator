#ifndef _CONN_H_
#define _CONN_H_
#include <netdb.h>
#define CONNMACRO

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
/* a collection of macros that are useful for this datagram prog */
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
    (a) = (((struct sockaddr_in *)(p)->ai_addr)->sin_addr).s_addr;             \
  } while (0)

/* from `struct sockaddr_storage` to sendto addr */
#define do_saddrsto_to_sin_addr(s, a)                                          \
  do {                                                                         \
    (a) = (((struct sockaddr_in *)(&(s)))->sin_addr).s_addr;                   \
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
  } while (0)

#define do_sender_findHS(p, hbuf, sbuf)                                        \
  do {                                                                         \
    if (getnameinfo((p)->ai_addr, (p)->ai_addrlen, (hbuf), sizeof((hbuf)),     \
                    (sbuf), sizeof((sbuf)),                                    \
                    NI_NUMERICHOST | NI_NUMERICSERV) != 0) {                   \
      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(errno));               \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define do_testkill(msg, kill, done)                                           \
  do {                                                                         \
    if (!(done)) {                                                             \
      if (!((done) |= (strcmp((msg), (kill)) == 0))) /* if not same string */  \
        (done) |= ((strncmp((msg), (kill), strlen((kill))) == 0) &&            \
                   (strlen((msg)) - strlen((msg)) == 1) &&                     \
                   ((msg)[strlen((kill))] == '\n'));                           \
    }                                                                          \
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

#define gen_rand(ret)                                                          \
  do {                                                                         \
    (ret) = (float)rand() / (float)(RAND_MAX + 1.0);                           \
  } while (0)

#define do_getSockFdFromRemoteSto(rAddr, rIP, s)                               \
  do {                                                                         \
    (s) =                                                                      \
        inet_ntop((rAddr).ss_family, get_in_addr((struct sockaddr *)&(rAddr),  \
                                                 (rIP), INET6_ADDRSTRLEN))     \
  } while (0)

#define do_getnameinfo(sa, sal, bhst, bsrv)                                    \
  do {                                                                         \
    if (getnameinfo((sa), (sal), (bhst), NI_MAXHOST, (bsrv), NI_MAXSERV,       \
                    NI_NUMERICHOST | NI_NUMERICSERV) != 0) {                   \
      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(errno));               \
      fprintf(stderr, "info: sockaddr:%p  socklen_t:%d, host:%s, serv:%s\n",   \
              (void *)(sa), (sal), (bhst), (bsrv));                            \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif /* _CONN_H_ */
