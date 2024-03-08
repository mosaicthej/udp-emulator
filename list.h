/*
 * Joseph Medernach, imy309, 11313955
 * John Miller, knp254, 11323966
 */

#ifndef _332_LIST_H_
#define _332_LIST_H_

#define MAX_LISTS 50
#define MAX_NODES 500


/*
 * NODEs are a type which hold a single data item, and are part of a list
 */
typedef struct _NODE {
    struct _NODE *prev; /* the previous item in the list */
    struct _NODE *next; /* the next item in the list */
    void* item; /* this node's item in the list */
} NODE;


/*
 * LISTs are a type which can hold a variable number of items of any single
 * type, there are a maximum number of LISTs which can be created and a
 * maximum number of items which can be held in all LISTs
 */
typedef struct _LIST {
    NODE *head; /* the first item in the list */
    NODE *tail; /* the last item in the list */
    NODE *curr; /* the current item in the list */
    int count; /* the number of items in the list */
    struct _LIST *next_list; /* used for organizing unused lists */
} LIST;


/*
 * A comparator is used to compare two items to determine (in)equality between
 * them
 *
 * precond: neither of the parameters are NULL
 * param: the parametes are the two items to compare, they should be the same
 *        type
 * return: 0 if the items do not match, 1 if they do
 */
typedef int (*comparator)(void*, void*);


/*
 * The itemFree type is used to free a type of items stored in a list
 *
 * precond: the parameter is not NULL
 * param: the parameter is a pointer to the item to be freed
 * postcond: the item passed is freed
 */
typedef void (*itemFree)(void*);


/*
 * The ListCreate procedure creates a new LIST and returns a pointer to it
 *
 * return: a new, empty, LIST, or NULL if creating a new list failed
 * postcond: an empty list is allocated
 *
 * This function may fail, if the maximum number of LISTs has been reached
 */
LIST *ListCreate();


/*
 * The ListAdd procedure adds a new item to the list immediately after the
 * current item and makes the new item the current item
 *
 * precond: the list param is not NULL
 * param: list - the list to add the item to
 * param: item - the item to add to the list
 * return: 0 if the item was successfully added to the list, -1 if not
 * postcond: an item is added to the list
 *
 * This procedure may fail, if the maximum number of items has be reached
 */
int ListAdd(LIST *list, void *item);


/*
 * The ListInsert procedure adds a new item to the list immediately before the
 * current item and makes the new item the current item
 *
 * precond: the list param is not NULL
 * param: list - the list to add the item to
 * param: item - the item to add to the list
 * return: 0 if the item was successfully added to the list, -1 if not
 * postcond: an item is added to the list
 *
 * This procedure may fail, if the maximum number of items has be reached
 */
int ListInsert(LIST *list, void *item);


/*
 * The ListAppend procedure adds a new item to the end of the list and makes
 * the new item the current item
 *
 * precond: the list param is not NULL
 * param: list - the list to add the item to
 * param: item - the item to add to the list
 * return: 0 if the item was successfully added to the list, -1 if not
 * postcond: an item is added to the list
 *
 * This procedure may fail, if the maximum number of items has be reached
 */
int ListAppend(LIST *list, void *item);


/*
 * The ListInsert procedure adds a new item to the start of the list and makes
 * the new item the current item
 * 
 * precond: the list param is not NULL
 * param: list - the list to add the item to
 * param: item - the item to add to the list
 * return: 0 if the item was successfully added to the list, -1 if not
 * postcond: an item is added to the list
 *
 * This procedure may fail, if the maximum number of items has be reached
 */
int ListPrepend(LIST *list, void *item);


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
void ListConcat(LIST *first, LIST *second);


/*
 * The ListCount procedure returns the number of items in a list
 *
 * precond: list is not NULL
 * param: list - the list to determine the number of items
 * return: the number of items in the list, -1 is the list is NULL
 */
int ListCount(LIST *list);


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
void *ListFirst(LIST *list);


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
void *ListLast(LIST *list);


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
void *ListNext(LIST *list);


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
void *ListPrev(LIST *list);


/*
 * The ListCurr procedure returns the current item of the list
 *
 * precond: list is not NULL
 * param: list - the list in question
 * return: the current item of the list, or NULL if the list is empty
 */
void *ListCurr(LIST *list);


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
void *ListSearch(LIST *list, comparator comp, void *item);


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
void *ListRemove(LIST *list);


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
void ListFree(LIST *list, itemFree item_free);


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
void *ListTrim(LIST *list);

#endif
