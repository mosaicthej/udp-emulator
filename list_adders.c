/*
 * Joseph Medernach, imy309, 11313955
 * John Miller, knp254, 11323966
 */

#include <list.h>
#include <string.h>

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
 * The ListAdd procedure adds a new item to the list immediately after the
 * current item and makes the new item the current item
 *
 * precond: the list param is not NULL
 * param: list - the list to add the item to
 * param: item - the item to add to the list
 * return: 0 if the item was successfully added to the list, -1 if not
 * postcond: an item is added to the list, the current item is changed
 *
 * This procedure may fail, if the maximum number of items has be reached
 */
int ListAdd(LIST *list, void *item) {
    NODE *new_node;

    /* if there are no more nodes available */
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

    /* link the new node */
    if (list->head == NULL) {
        new_node->prev = NULL;
        new_node->next = NULL;
        list->head = new_node;
        list->tail = new_node;

    } else {
        /* if there is already a node after the current node */
        if ((list->curr)->next != NULL) {
            ((list->curr)->next)->prev = new_node;
            new_node->next = (list->curr)->next;
        } else { /* if not we are inserting at the end */
            list->tail = new_node;
        }
        new_node->prev = list->curr;
        (list->curr)->next = new_node;
    }
    list->curr = new_node;

    return 0;
}


/*
 * The ListInsert procedure adds a new item to the list immediately before the
 * current item and makes the new item the current item
 *
 * precond: the list param is not NULL
 * param: list - the list to add the item to
 * param: item - the item to add to the list
 * return: 0 if the item was successfully added to the list, -1 if not
 * postcond: an item is added to the list and the current item is changed
 *
 * This procedure may fail, if the maximum number of items has be reached
 */
int ListInsert(LIST *list, void *item) {
    NODE *new_node;

    /* if there are no more nodes available */
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

    /* link the new node */
    if (list->head == NULL) {
        new_node->prev = NULL;
        new_node->next = NULL;
        list->head = new_node;
        list->tail = new_node;

    } else {
        /* if there is already a node before the current node */
        if ((list->curr)->prev != NULL) {
            ((list->curr)->prev)->next = new_node;
            new_node->prev = (list->curr)->prev;
        } else { /* if not we are inserting at the start */
            list->head = new_node;
        }
        new_node->next = list->curr;
        (list->curr)->prev = new_node;
    }
    list->curr = new_node;

    return 0;
}


/*
 * The ListAppend procedure adds a new item to the end of the list and makes
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
int ListAppend(LIST *list, void *item) {
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
    if (list->head == NULL) {
        new_node->prev = NULL;
        list->head = new_node;
    /* if the list has at least one item */
    } else {
        new_node->prev = list->tail;
        list->tail->next = new_node;
    }
    list->tail = new_node;
    new_node->next = NULL;
    list->curr = new_node;

    return 0;
}


/*
 * The ListInsert procedure adds a new item to the start of the list and makes
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


/*
 * The ListConcat procedure adds the contents of one list to the end of
 * another list and deletes the now empty list
 *
 * precond: the first param is not NULL
 * precond: the second param is not NULL
 * precond: both list params are not the same
 * param: first - the list which will have the second's contents appended
 *                to it
 * param: second - the list whose contents will be appended to first, and will
 *                 be deleted
 * postcond: the second list is deleted, the first list has the second's
 *           content appended to it
 *
 * if the first list is empty, the current item is set to the new first item
 * in the first list
 */
void ListConcat(LIST *first, LIST *second) {
    /* if a NULL list was passed or params are the same */
    if (first == NULL || second == NULL || first == second) {
        return;
    }

    /* if second is not empty, add it to the end of first */
    if (second->head != NULL) {
        /* if first is empty */
        if (first->head == NULL) {
            first->head = second->head;
            first->curr = first->head;
        } else { /* else first is not empty */
            first->tail->next = second->head;
            second->head->prev = first->tail;
        }
        first->tail = second->tail;
        first->count += second->count;
    }

    /* delete second */
    second->head = NULL;
    second->tail = NULL;
    second->curr = NULL;
    second->count = 0;
    second->next_list = free_list;
    free_list = second;
    num_lists--;
}
