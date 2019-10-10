#include <stdio.h>
#include <stdlib.h>
#include <linkedlist.h>

const size_t linkedlist_elem_size = sizeof(linkedlist_elem);

linkedlist_elem *linkedlist_new(linkedlist_elem *list, void *data) {
	linkedlist_elem *new = (linkedlist_elem*)malloc(linkedlist_elem_size);
	new->ptr = data;
	new->prev = NULL;
	new->next = NULL;
	if(list != NULL) {
		new->next = list;
		list->prev = new;
	} 
	return new;

}

void *linkedlist_search(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg) {
	linkedlist_elem *curr = list;
	if (!curr) return NULL;
	while (curr != NULL) {
		if (curr->ptr) {
			if (fun((const void*)(curr->ptr), (void*)arg)) {
				return curr->ptr;
			}
		}
		curr = curr->next;
	}
	return NULL;
}

linkedlist_elem *linkedlist_delete(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg) {
	linkedlist_elem *curr = list;
	while (curr != NULL) {
		linkedlist_elem *next = curr->next;
		if (fun((const void*)(curr->ptr), (void*)arg)) { //0: prev ok, next = NULL
			if (curr->prev && curr->next) {
				(curr->prev)->next = curr->next;	
				(curr->next)->prev = curr->prev;
			} else if (!curr->prev) list = curr->next;
			else if (!curr->next) list = curr->prev; 
			free(curr->ptr);
			curr->ptr = NULL;
			free(curr);
			curr = NULL;
			return list;
		}
		curr = next;
	}
	return list;	//failure
}

void linkedlist_free(linkedlist_elem *list) {	//WARNING: USING linkedlist WITH STACK ELEMENTS BLOWS THINGS UP, DON'T DO IT
	if (list == NULL) return;
	if (list->next != NULL) linkedlist_free(list->next);
	free(list->ptr);
	free(list);
}
