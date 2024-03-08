/*
 * Joseph Medernach, imy309, 11313955
 * John Miller, knp254, 11323966
 */

#include <list.h>
#include <stddef.h>

    extern int num_nodes;
    extern NODE *free_node;

    extern int num_lists;
    extern LIST *free_list;
/*
 * the ListRemove procedure removes the current item from the list and returns
 * it, the new current item is the item after the deleted item (or the new
 * last item if the deleted item was the last in the list)
 *
 * precond: the list param is not NULL
 * param: list - the list to delete from
 * return: the deleted item, or NULL if the list is empty
 * postcond: the deleted item is no longer in the list, the current item is
 *           changed
 *
 * if the list is empty then nothing is done and NULL is returned
 * if the list is empty after this operation then the new current item is NULL
 */
void *ListRemove(LIST *list) {
    void *item;
    NODE *delete_node;

    /* if list is NULL or if the list is empty */
    if (list == NULL || list->curr == NULL) {
        return NULL;
    }

    /* save the current item to return later */
    item = list->curr->item;
    list->count--;
    delete_node = list->curr;

    /* relink node chain */
    /* the item is the first item */
    if (list->curr->prev == NULL) {
        list->head = list->curr->next;
    } else {
        list->curr->prev->next = list->curr->next;
    }

    /* the item is the last item */
    if (list->curr->next == NULL) {
        list->tail = list->curr->prev;
        list->curr = list->curr->prev;
    } else {
        list->curr->next->prev = list->curr->prev;
        list->curr = list->curr->next;
    }

    /* delete the node */
    num_nodes--;
    delete_node->item = NULL;
    delete_node->prev = NULL;
    delete_node->next = free_node;
    free_node = delete_node;

    return item;
}


/*
 * The listFree procedure deletes a list and frees every item in it
 *
 * precond: the list param is not NULL
 * precond: the item_free param is not NULL
 * param: list - the list to delete
 * param: item_free - the procedure to use to free the lists items
 *
 * the list should no longer be accessed after this operation
 */
void ListFree(LIST *list, itemFree item_free) {
    NODE *current;
    NODE *next_current;

    if (list == NULL || item_free == NULL) {
        return;
    }

    /* for each node in the list */
    for (current = list->head; current != NULL; current = next_current) {
        /* free the items in the current node */
        (*item_free)(current->item);
        current->item = NULL;
        current->prev = NULL;

        /* add the current node to the free_node stack*/
        next_current = current->next;
        current->next = free_node;
        free_node = current;
    }

    /* de-allocate the list */
    list->head = NULL;
    list->tail = NULL;
    list->curr = NULL;
    num_nodes -= list->count;
    list->count = 0;
    num_lists--;
    list->next_list = free_list;
    free_list = list;
}


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
