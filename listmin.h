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

#ifndef _LIST_MIN_H_
#define _LIST_MIN_H_

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
 * The ListCreate procedure creates a new LIST and returns a pointer to it
 *
 * return: a new, empty, LIST, or NULL if creating a new list failed
 * postcond: an empty list is allocated
 *
 * This function may fail, if the maximum number of LISTs has been reached
 */
LIST *ListCreate(void);

/*
 * The ListPrepend procedure adds a new item to the start of the list and makes
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
 * The ListCount procedure returns the number of items in a list
 *
 * precond: list is not NULL
 * param: list - the list to determine the number of items
 * return: the number of items in the list, -1 is the list is NULL
 */
int ListCount(LIST *list);

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

#endif /* _LIST_MIN_H_ */
