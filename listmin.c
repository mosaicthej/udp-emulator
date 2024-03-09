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

#include <listmin.h>
#include <string.h>
#include <stddef.h>

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

/*
 * The ListCreate procedure creates a new LIST and returns a pointer to it
 *
 * return: a new, empty, LIST, or NULL if creating a new list failed
 * postcond: an empty list is allocated
 *
 * This function may fail, if the maximum number of LISTs has been reached
 */
LIST *ListCreate() {
    LIST *new_list;

    /* init the nodes and lists if not yet done for the first time */
    if (!inited) {
        memset(&node_array, 0, MAX_NODES*sizeof(NODE));
        memset(&list_array, 0, MAX_LISTS*sizeof(LIST));
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
    new_list->curr = NULL;
    new_list->count = 0;
    new_list->next_list = NULL;
    return new_list;
}


/*
 * The ListPrepend procedure adds a new item to the start of the list and makes
 * the new item the current item
 * 
 * precond: the list param is not NULL
 * param: list - the list to add the item to
 * param: item - the item to add to the list
 * return: 0 if the item was successfully added to the list, -1 if not
 * postcond: an item is added to the list and the current item is changed
 *
 * This procedure may fail, if the maximum number of items has be reached
 */
int ListPrepend(LIST *list, void *item) {
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
    list->curr = new_node;

    return 0;
}


/* Things from `list_movers.c` */

/*
 * The ListCount procedure returns the number of items in a list
 *
 * precond: list is not NULL
 * param: list - the list to determine the number of items
 * return: the number of items in the list, -1 if the list is NULL
 */
int ListCount(LIST *list) {
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

/*
 * The ListTrim procedure deletes the last item from the list and returns it
 * the new current item is the new last item
 *
 * precond: the list param is not NULL
 * param: list - the list to delete from
 * return: the delted item, or NULL if the list is empty
 * postcond: an item is removed from the list, the current item is changed
 *
 * if the list is empty then nothing is done and NULL is returned
 * if the list is empty after the procedure then the new current item is NULL
 */
void *ListTrim(LIST *list) {
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

    list->curr = list->tail->prev;
    list->tail = list->tail->prev;
    /* relink node chain */
    /* the item is the only item in the list */
    if (list->curr == NULL) {
        list->head = NULL;
    } else {
        list->curr->next = NULL;
    }

    /* add the free node to the stack */
    num_nodes--;
    delete_node->item = NULL;
    delete_node->prev = NULL;
    delete_node->next = free_node;
    free_node = delete_node;

    return item;
}
