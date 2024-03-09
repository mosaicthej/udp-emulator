#include <stdio.h>
#include <listmin.h>
#include <stdlib.h>

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

  printf("All tests passed\n");

  return 0;

}
