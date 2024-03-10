#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int main(void){
  
  /*  trying to change a host name + port => getaddrinfo => canonical name:
   *  
   *  I think it would convert "localhost"=> "127.0.0.1"
   *  */
  struct addrinfo hints, *tempAI;
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  int s;

  hints.ai_family = AF_INET;

  s = getaddrinfo("localhost", "12345", NULL, &tempAI);
  printf("s: %d\n", s);
  if (s != 0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return 1;
  }
  
  s = getnameinfo(tempAI->ai_addr, tempAI->ai_addrlen, 
    hbuf, NI_MAXHOST, 
    sbuf, NI_MAXSERV, 
    NI_NUMERICHOST | NI_NUMERICSERV);

  printf("s: %d\n", s);
  printf("host: %s, \tserv: %s\n", hbuf, sbuf);


  return 0;
  

}
