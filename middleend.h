#ifndef _MIDDLEEND_H_
#define _MIDDLEEND_H_

#include <netdb.h>
#include <stdint.h>
#imclude <queue.h>
#include <list.h>

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

#define PORTMAX 65535
#define PORTMIN 1024

#define MAX_MSG_SIZE 256
#define MAX_ADDR_LEN 128
#define MAX_PORT_LEN 32

#define MAX_DELAY 10 /* seconds */

typedef struct edptInfo{
  char hostName[NI_MAXHOST]; /* Hostname, we only support 1 hostname here */ 
  char portRcv[NI_MAXSERV]; /*  port used for receiving */
  char portSnd[NI_MAXSERV]; /* port used for sending */
} EndPointName; /* this is 1 row on the table */

typedef struct lookupTable{
  EndPointName left; /* left endpoint */
  EndPointName right; /* right endpoint */
} LookupTable; /* this is the table */

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

  LookupTable *nameTbl;
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

  LookupTable *nameTbl;
} Receiver_info;
/* TODO: kill message should also be an argument
add it to sender info and receiver info, also refactor the routines. */

typedef struct _channelMsg {
  struct sockaddr * fromAddr; /* fill with `theirAddr` */
  char *msg;
} ChannelMsg;

void *send_thread(void *);
void *receive_thread(void *);

int checkArgs(char*, char*, char*, char*, char*);

struct addrinfo * pickToSend( struct addrinfo *,
      struct addrinfo *,
      struct sockaddr *);


#endif /* _MIDDLEEND_H_ */
