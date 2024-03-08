/*
 * Joseph Medernach, imy309, 11313955
 * John Miller, knp254, 11323966
 */

#include <list.h>
#include <stddef.h>

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


/*
 * The ListFirst procedure makes the current item of the list the first item
 *
 * precond: list is not NULL
 * param: list - the list in question
 * return: the first item in the list, or NULL if the list is empty
 * postcond: the current item of the list is changed
 *
 * if the list is empty, nothing is done and NULL is returned
 */
void *ListFirst(LIST *list) {
    /* if the list is empty or if a NULL list was passed */
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    list->curr = list->head;
    return list->curr->item;
}


/*
 * The ListLast procedure makes the current item of the list the last item
 *
 * precond: list is not NULL
 * param: list - the list in question
 * return: the last item in the list, or NULL if the list is empty
 * postcond: the current item of the list is changed
 *
 * if the list is empty, nothing is done and NULL is returned
 */
void *ListLast(LIST *list) {
    /* if the list is empty or if a NULL list was passed */
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    list->curr = list->tail;
    return list->curr->item;
}


/*
 * The ListNext procedure makes the current item of the list the item after
 * the current item
 *
 * precond: list is not NULL
 * param: list - the list in question
 * return: the new current item of the list, or NULL if the list is empty or
 *         this operation would attempt to move the current item beyond
 *         the end of the list
 * postcond: the current item of the list is changed
 *
 * if the list is empty, nothing is done and NULL is returned
 * if this operation would move the current item beyond the end of the list
 * nothing is done and NULL is returned
 */
void *ListNext(LIST *list) {
    /* if a NULL list was passed or if the list is empty or */
    /* if this pushes the current item past the end of the list */
    if (list == NULL || list->head == NULL || list->curr->next == NULL) {
        return NULL;
    }

    list->curr = list->curr->next;
    return list->curr->item;
}


/*
 * The ListPrev procedure makes the current item of the list the item before
 * the current item
 *
 * precond: list is not NULL
 * param: list - the list in question
 * return: the new current item of the list, or NULL if the list is empty or
 *         this operation would attempt to move the current item beyond
 *         the start of the list
 * postcond: the current item of the list is changed
 *
 * if the list is empty, nothing is done and NULL is returned
 * if this operation would move the current item beyond the start of the list
 * nothing is done and NULL is returned
 */
void *ListPrev(LIST *list) {
    /* if a NULL list was passed or if the list is empty */
    /* if this pushes the current item before the start of the list */
    if (list == NULL || list->head == NULL || list->curr->prev == NULL) {
        return NULL;
    }

    list->curr = list->curr->prev;
    return list->curr->item;
}


/*
 * The ListCurr procedure returns the current item of the list
 *
 * precond: list is not NULL
 * param: list - the list in question
 * return: the current item of the list, or NULL if the list is empty
 */
void *ListCurr(LIST *list) {
    /* if a NULL list was passed */
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    return list->curr->item;
}


/*
 * The ListSearch procedure searches the list for a target item starting from
 * the current item
 *
 * precond: the list param is not NULL
 * precond: the comp param is not NULL
 * param: list - the list to search
 * param: comp - the function used to compare items
 * param: item - the item to search for
 * return: the item found in the list, or NULL if the list is empty or no item
 *         was found
 * postcond: the current item is set to be the item found, or the last item
 *           in the list if no item was found
 *
 * if the list is empty then nothing is done and NULL is returned
 */
void *ListSearch(LIST *list, comparator comp, void *item) {
    if (list == NULL || comp == NULL) {
        return NULL;
    }

    /* while there is a current item, check if it is the one we want */
    /* if not, look at the next item */
    while (list->curr != NULL) {
        if ((*comp)(list->curr->item, item)) {
            return list->curr->item;
        }
        list->curr = list->curr->next;
    }

    /* if the item was not found then the current item should be the last */
    /* item */
    list->curr = list->tail;
    return NULL;
}
