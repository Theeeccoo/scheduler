/*
 * Copyright(C) 2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * list.h - Private linked list library.
 */
 
#ifndef LIST_H_
#define LIST_H_

	#include <stdlib.h>
	#include <stdio.h>

/*============================================================================*
 *                            Linked List Library                             *
 *============================================================================*/
 
	/*
	 * List node.
	 */
	struct node
	{
		int x;             /* Number.                */
		struct node *next; /* Next node in the list. */
	};

	/*
	 * Linked list.
	 */
	struct list
	{
		int length;       /* List length. */
		struct node head; /* Head node.   */
		struct node tail; /* Tail node.   */
	};

	/*
	 * Casters to a list pointer.
	 */
	#define LISTP(l) \
		((struct list *)(l))

	/*
	 * Creates a list of numbers.
	 */
	extern struct list *list_create(void);
	
	/*
	 * Destroys a list of numbers.
	 */
	extern void list_destroy(struct list *l);

	/*
	 * Asserts if a list is empty.
	 */
	#define list_empty(l) \
		(LISTP(l)->head.next == &LISTP(l)->tail)
		
	/*
	 * Returns the length of a list.
	 */
	#define list_length(l) \
		(LISTP(l)->length) \

	/*
	 * Pushes a number in a list.
	 */
	extern void list_push(struct list *l, int x);
	
	/*
	 * Pops a number from a list.
	 */
	extern int list_pop(struct list *l);

#endif
