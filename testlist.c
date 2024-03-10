/* Mark Jia, mij623, 11271998  */
#include <stdio.h>
#ifdef TESTLISTMIN
#include <listmin.h>
#elif TESTQUEUE
#include <queue.h>
#else
#include <list.h> /* default to original list if unspecified */
#endif /* lol I love this, macro is a paradigm lol */
#include <stdlib.h>

#ifdef TESTQUEUE 
/* testing the queues */
#define LIST QUEUE /* alias now*/
#define ListCreate QueueCreate
#define ListPrepend QEnqueue
#define ListTrim QDequeue
#define ListCount QCount
#endif

#define safe_res(x) if (res != x) { printf("Error: res = %d\n", res); exit(1);}
#define safe_resP(x) if (x==NULL) { printf("Error: resP = NULL\n"); exit(1); }

int main(void){
  LIST * l0;
  int x, y, z;
  char * a, * b, * c;

  int res; void * resP;

  l0 = ListCreate();

  x = 1, y = 2, z = 3;
  a = "moo", b = "cow", c = "milk";

  res = ListPrepend(l0, &x); safe_res(0);/* add 1 */
  res = ListPrepend(l0, &y); safe_res(0);/* add 2 */
  res = ListPrepend(l0, a); safe_res(0);/* add moo */

  resP = ListTrim(l0); safe_resP(resP);/* 1 */
  printf("expected 1, resP = %d\n", *(int *)resP);
  resP = ListTrim(l0); safe_resP(resP);/* 2 */
  printf("expected 2, resP = %d\n", *(int *)resP);

  res = ListPrepend(l0, &z); safe_res(0);/* add 3 */
  resP = ListTrim(l0); safe_resP(resP);/* moo */
  printf("expected moo, resP = %s\n", (char *)resP);

  resP = ListTrim(l0); safe_resP(resP);/* 3 */
  printf("expected 3, resP = %d\n", *(int *)resP);

  resP = ListTrim(l0);
  if (resP==NULL) printf("expected got to end, yes it is\n");

  resP = ListTrim(l0);
  if (resP==NULL) printf("expected got to end (2), yes it is\n");

  res = ListPrepend(l0, b); safe_res(0);/* add cow */
  res = ListPrepend(l0, c); safe_res(0);/* add milk */
  if (ListCount(l0) != 2) printf("Error: ListCount = %d\n", ListCount(l0));
  resP = ListTrim(l0); safe_resP(resP);/* cow */
  printf("expected cow, resP = %s\n", (char *)resP);
  resP = ListTrim(l0); safe_resP(resP);/* milk */
  printf("expected milk, resP = %s\n", (char *)resP);


  printf("All tests passed\n");

  return 0;

}

#ifdef TESTQUEUE /* remove aliases */
/* testing the queues */
#undef LIST
#undef ListCreate 
#undef ListPrepend 
#undef ListTrim 
#undef ListCount 
#endif
