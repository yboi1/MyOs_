#include "list.h"
#include "interrupt.h"
#include "print.h"
#include "stdint.h"
#include "console.h"

void list_init(struct list * list) {
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

// 将elem放入链表前
void list_insert_before(struct list_elem* before, struct list_elem* elem) {
    enum intr_status old_status = intr_disable();

    elem->prev = before->prev;
    elem->next = before;

    before->prev->next = elem;
    before->prev = elem;
    intr_set_status(old_status);
    
}

void list_push(struct list *plist, struct list_elem *elem) {
    list_insert_before(plist->head.next, elem);
}

void list_append(struct list *plist, struct list_elem *elem) {
    list_insert_before(&plist->tail, elem);
}

void list_remove(struct list_elem *pelem) {
    enum intr_status old_status = intr_disable();

    pelem->next->prev = pelem->prev;
    pelem->prev->next = pelem->next;

    intr_set_status(old_status);
}

struct list_elem* list_pop(struct list* plist) {
    struct list_elem* elem = plist->head.next;
    list_remove(elem);
    return elem;
}

bool elem_find(struct list *plist, struct list_elem *obj_elem) {
    struct list_elem* elem = plist->head.next;
    while (elem != &plist->tail) {
        if(elem == obj_elem) {
            return true;
        }
        elem = elem->next;
    }
    return false;
}

struct list_elem* list_traversal(struct list* plist, function func, int arg) {
    struct list_elem* elem = plist->head.next;
    if(list_empty(plist)) {
        return NULL;
    }
    while(elem != &plist->tail) {
        if(func(elem, arg)) {
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}

uint32_t list_len(struct list *plist) {
    struct list_elem* elem = plist->head.next;
    uint32_t length = 0;
    while(elem != &plist->tail) {
        length++;
        elem = elem->next;
    }
    return length;
}

bool list_empty(struct list *plist) {
    return (plist->head.next == &plist->tail ? true : false);
}
