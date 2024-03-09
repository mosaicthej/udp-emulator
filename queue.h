/*
 * Joseph Medernach, imy309, 11313955
 * John Miller, knp254, 11323966
 */

/*
 * Mark Jia, mij623, 11271998
 * Modified list.h to John's original list lib.
 * Only keep the necessary ones, limiting list only work with
 * ListPrepend() and ListTrim()
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#define MAX_LISTS 50
#define MAX_NODES 500

#define LIST QUEUE /* we talk about QUEUE here !*/
#undef HASCURR     /* there is no current item in a QUEUE */

/*
 * NODEs are a type which hold a single data item, and are part of a list
 */
typedef struct _NODE NODE;
/* forward incomplete definition to hide it from users
 * (me, primarily, if anyone else, to prevent commiting crimes) */

/*
 * LISTs are a type which can hold a variable number of items of any single
 * type, there are a maximum number of LISTs which can be created and a
 * maximum number of items which can be held in all LISTs
 */
typedef struct _LIST {
  NODE *head; /* the first item in the list */
  NODE *tail; /* the last item in the list */
#ifdef HASCURR
  NODE *curr; /* the current item in the list */
#endif

  int count;               /* the number of items in the list */
  struct _LIST *next_list; /* used for organizing unused lists */
} LIST;

/* QueueCreate()
 *
 * Creates a new QUEUE and returns a pointer to it.
 *
 * return: a new, empty, QUEUE, or NULL if creating a new list failed
 * postcond: an empty list is allocated
 *
 * This function may fail, if the maximum number of LISTs has been reached
 */
LIST *QueueCreate(void) ;
 

/*
 * The QEnqueue procedure adds a new item to the end of the queue,
 * sets the new item as the current item
 *
 * @param: - `queue`: pointer to a QUEUE. Cannot be NULL.
 * @param: - `item`: pointer to the item to be added to the queue.
 *
 * @ret: - 0 if the item was successfully added to the queue, -1 if not
 *
 * note: This procedure may fail,
 * if the maximum number of the items has been reached
 *
 * */
int QEnqueue(LIST *queue, void *item) ;

/* QCount:
 *
 * The QueueCount procedure returns the number of items in a queue
 *
 * @param: - `queue`: pointer to a QUEUE. Cannot be NULL.
 *
 * @ret: - the number of items in the queue, -1 if the queue is NULL
 *
 */
int QCount(LIST *list);

/* QDequeue:
 * The QDequeue procedure deletes the first item from the queue and returns it
 *
 * @param: - `queue`: pointer to a QUEUE. Cannot be NULL.
 *
 * @ret: - the deleted item, or NULL if the queue is empty
 * */
void *QDequeue(LIST *list);
#endif /* _QUEUE_H_ */
