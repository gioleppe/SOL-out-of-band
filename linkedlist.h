#ifndef LINKED_LIST_H
	typedef struct linkedlist_elem {
		void *ptr;
		struct linkedlist_elem *prev;
		struct linkedlist_elem *next;
	} linkedlist_elem;

	const size_t linkedlist_elem_size;

	linkedlist_elem *linkedlist_new(linkedlist_elem *list, void *data);

	void *linkedlist_search(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg);

	linkedlist_elem *linkedlist_delete(linkedlist_elem *list, int (*fun)(const void*, void*), void *arg);

	void linkedlist_free(linkedlist_elem *list);
	
	#define LINKED_LIST_H
#endif

