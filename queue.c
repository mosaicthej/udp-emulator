/*
 * Joseph Medernach, imy309, 11313955
 * John Miller, knp254, 11323966
 */

/*
 * Mark Jia, mij623, 11271998
 * Modified list.h to John's original list lib.
 * Only keep the necessary ones that is used within this scope
 *
 * - ListCreate, - ListPrepend, - ListCount, - ListTrim
 *
 * This may go against the point of having a library but lol
 * I can argue that it would reduce static link bloating,
 * and reducing the chance of me being too blind using wrong stuff...
 *
 * In this case, there would be no point of separating into multiple files
 * * */

/*
 * New idea!
 * It's more straightforward and safe if I just rename it "queue"!
 * MACROS go brrrrrrr....
 *
 * Also make everything thread safe! (with pthreads)
 * */

#include <pthread.h>
#include <queue.h>
#include <stddef.h>
#include <string.h>

#define LIST QUEUE /* alias now*/
#undef HASCURR     /* no need for curr */

/*
 * NODEs are a type which hold a single data item, and are part of a list
 */
struct _NODE {
  struct _NODE *prev; /* the previous item in the list */
  struct _NODE *next; /* the next item in the list */
  void *item;         /* this node's item in the list */
};

/* Things from `list_adders.c`*/

/* keeping track of it the user has created any lists yet */
int inited = 0;

/* the statically allocated node arrays */
NODE node_array[MAX_NODES];
NODE *free_node;
int num_nodes;

/* the statically allocated list arrays */
LIST list_array[MAX_LISTS];
LIST *free_list;
int num_lists;

/* QueueCreate()
 *
 * Creates a new QUEUE and returns a pointer to it.
 *
 * return: a new, empty, QUEUE, or NULL if creating a new list failed
 * postcond: an empty list is allocated
 *
 * This function may fail, if the maximum number of LISTs has been reached
 */
LIST *QueueCreate() {
  LIST *new_list;

  /* init the nodes and lists if not yet done for the first time */
  if (!inited) {
    memset(&node_array, 0, MAX_NODES * sizeof(NODE));
    memset(&list_array, 0, MAX_LISTS * sizeof(LIST));
    free_node = node_array;
    free_list = list_array;
    num_nodes = 0;
    num_lists = 0;
    inited = 1;
  }
  /* if there are no lists remaining */
  if (free_list == NULL) {
    return NULL;
  }
  new_list = free_list;
  num_lists++;
  /* advance the free_list* to the next free list if it exists */
  if (num_lists == MAX_LISTS) {
    free_list = NULL;
  } else if (free_list->next_list == NULL) {
    free_list++;
  } else {
    free_list = free_list->next_list;
  }

  /* ensure that the list is fresh */
  new_list->head = NULL;
  new_list->tail = NULL;
#ifdef HASCURR
  new_list->curr = NULL;
#endif
  new_list->count = 0;
  new_list->next_list = NULL;
  return new_list;
}

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
int QEnqueue(LIST *list, void *item) {
  NODE *new_node;

  /* if the maximum number of nodes has already been created */
  /* if a NULL list was passed */
  if (list == NULL || num_nodes == MAX_NODES) {
    return -1;
  }

  new_node = free_node;
  num_nodes++;

  /* advance the free_node* to the next free node if one exists */
  if (num_nodes == MAX_NODES) {
    free_node = NULL;
  } else if (free_node->next == NULL) {
    free_node++;
  } else {
    free_node = free_node->next;
    new_node->next = NULL;
  }

  new_node->item = item;
  list->count++;

  /* link the node */
  /* if the list has no items */
  if (list->tail == NULL) {
    new_node->next = NULL;
    list->tail = new_node;
    /* if the list has at least one item */
  } else {
    new_node->next = list->head;
    list->head->prev = new_node;
  }
  list->head = new_node;
  new_node->prev = NULL;
#ifdef HASCURR
  list->curr = new_node;
#endif

  return 0;
}

/* Things from `list_movers.c` */

/* QCount:
 *
 * The QueueCount procedure returns the number of items in a queue
 *
 * @param: - `queue`: pointer to a QUEUE. Cannot be NULL.
 *
 * @ret: - the number of items in the queue, -1 if the queue is NULL
 *
 */
int QCount(LIST *list) {
  /* if a NULL list was passed */
  if (list == NULL) {
    return -1;
  }
  return list->count;
}

/* Things from `list_removers.c`
 *
 * Assuming I never malloc...
 * (TODO: if I ever malloc for items in List,
 * need to add `ListFree` here too)
 * */

/* QDequeue:
 * The QDequeue procedure deletes the first item from the queue and returns it
 *
 * @param: - `queue`: pointer to a QUEUE. Cannot be NULL.
 *
 * @ret: - the deleted item, or NULL if the queue is empty
 * */
void *QDequeue(LIST *list) {
  void *item;
  NODE *delete_node;

  /* if list is NULL or if the list is empty */
  if (list == NULL || list->tail == NULL) {
    return NULL;
  }

  /* save the current item to return later */
  item = list->tail->item;
  list->count--;
  delete_node = list->tail;
#ifdef HASCURR
  list->curr = list->tail->prev;
#endif
  list->tail = list->tail->prev; /* narrow the tail */
/* relink node chain */
/* the item is the only item in the list */
#ifndef HASCURR   /* which, it is */
#define curr tail /* replace with tail */
#endif
  if (list->curr == NULL) {
    list->head = NULL;
  } else {
    list->curr->next = NULL;
  }
#ifndef HASCURR
#undef curr
#endif

  /* add the free node to the stack */
  num_nodes--;
  delete_node->item = NULL;
  delete_node->prev = NULL;
  delete_node->next = free_node;
  free_node = delete_node;

  return item;
}
