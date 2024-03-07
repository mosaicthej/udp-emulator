#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
  char *send_to_host;
  char *send_to_port;
};

struct receiver_info {
  char *receive_from_port;
};

/* validate port numbers */
int check_args(char* st_host, char* st_port, char* rf_port){
  int st_p = atoi(st_port);
  int rf_p = atoi(rf_port);

  if (strlen(st_host) == 0){
    fprintf(stderr, "send-to-host cannot be empty\n");
    return EXIT_FAILURE;
  }
  
  if(st_p < PORTMIN || st_p > PORTMAX){
    fprintf(stderr, "send-to-port must be between %d and %d\n", 
            PORTMIN, PORTMAX);
    return EXIT_FAILURE;
  }
 
  if(rf_p < PORTMIN || rf_p > PORTMAX){
    fprintf(stderr, "receive-from-port must be between %d and %d\n", 
            PORTMIN, PORTMAX);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

  }
}

