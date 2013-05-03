/*
 * Linux Event Logging for the Enterprise
 * Copyright (c) International Business Machines Corp., 2001
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 *  Please send e-mail to lkessler@users.sourceforge.net if you have
 *  questions or comments.
 *
 *  Project Website:  http://evlog.sourceforge.net/
 *
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "evl_list.h"

/*
 * Generic functions for a doubly-linked list.
 * We use a circular list, which makes traversal slightly more tricky,
 * but makes it easy to append (insert it at head->li_prev), and allows
 * us to use zero space for an empty list (the header is part of the list).
 */
evl_listnode_t *
_evlAllocListNode()
{
	evl_listnode_t *p = (evl_listnode_t*) malloc(sizeof(evl_listnode_t));
	assert(p != NULL);
	(void) memset(p, 0, sizeof(evl_listnode_t));
	return p;
}

evl_listnode_t *
_evlMkListNode(const void *data)
{
	evl_listnode_t *p = _evlAllocListNode();
	p->li_data = data;
	p->li_next = p;
	p->li_prev = p;
	return p;
}

/*
 * head can be NULL.  Returns pointer to (possibly new) list head.
 */
evl_list_t *
_evlAppendToList(evl_list_t *head, const void *data)
{
	evl_listnode_t *p = _evlMkListNode(data);
	if (head) {
		evl_listnode_t *tail = head->li_prev;
		p->li_next = head;
		p->li_prev = tail;
		head->li_prev = p;
		tail->li_next = p;
		return head;
	} else {
		p->li_next = p;
		p->li_prev = p;
		return p;
	}
}

/* Returns the number of nodes in the list. */
int
_evlGetListSize(const evl_list_t *head)
{
	evl_listnode_t *p = (evl_listnode_t*) head;
	int n = 0;

	if (!head) {
		return 0;
	}
	
	do {
		n++;
		p = p->li_next;
	} while (p != head);
	return n;
}

/*
 * head is the head of a list of objects.  The first field of each object is
 * a char* pointing to the object's name.  Return the address of the LIST NODE
 * for the object with the indicated name.
 */
evl_listnode_t *
_evlFindNamedItemInList(const evl_list_t *head, const char *name)
{
	evl_listnode_t *p = (evl_listnode_t*) head;
	const char *pname, **pdata;

	assert(name != NULL);
	if (!head) {
		return NULL;
	}

	do {
		pdata = (const char **) p->li_data;
		pname = *pdata;
		if (pname && !strcmp(pname, name)) {
			return p;
		}
		p = p->li_next;
	} while (p != head);
	return NULL;
}

/*
 * Free all the nodes in the list.  If freeData is non-zero, also free
 * the data item each node points to.
 */
void
_evlFreeList(evl_list_t *head, int freeData)
{
	evl_listnode_t *next, *p = head;
	if (!head) {
		return;
	}

	do {
		if (freeData) {
			free((void*) p->li_data);
		}
		/*
		 * Avoid ref to p->li_next after p has been freed (and
		 * potentially re-malloced).
		 */
		next = p->li_next;
		free(p);
		p = next;
		/* BEAM complains "Using pointer to deallocated memory" here. 
		 * This warning can be ignored.
		 */
	} while (p != head);
}

/*
 * Return a pointer to the nth node in the list, where the first (head)
 * node is 0.
 */
evl_listnode_t *
_evlGetNthNode(evl_list_t *head, int n)
{
	evl_listnode_t *p = head;
	int i = 0;
	if (!head) {
		return NULL;
	}

	do {
		if (i == n) {
			return p;
		}
		i++;
		p = p->li_next;
	} while (p != head);
	return NULL;
}

/*
 * Return a pointer to the value of the nth node in the list, where the first
 * (head) node is 0.
 */
void *
_evlGetNthValue(evl_list_t *head, int n)
{
	evl_listnode_t *p = _evlGetNthNode(head, n);
	return (void*) (p ? p->li_data : NULL);
}

/*
 * Insert node insertMe into the list in front of beforeMe.
 */
void
_evlInsertListNode(evl_listnode_t *insertMe, evl_listnode_t *beforeMe)
{
	evl_listnode_t *afterMe = beforeMe->li_prev;
	insertMe->li_next = beforeMe;
	insertMe->li_prev = afterMe;
	afterMe->li_next = insertMe;
	beforeMe->li_prev = insertMe;
}

/*
 * Create a list node with data insertMe, and insert it into the list pointed
 * to by list, in front of node beforeMe.  Returns a pointer to the (possibly
 * new) head of the list.
 */
evl_list_t *
_evlInsertToList(void *insertMe, evl_listnode_t *beforeMe, evl_list_t *list)
{
	evl_listnode_t *newNode = _evlMkListNode(insertMe);
	assert(beforeMe != NULL);
	_evlInsertListNode(newNode, beforeMe);
	if (beforeMe == list) {
		/* Inserting at head of list. */
		return newNode;
	} else {
		return list;
	}
}

/*
 * Remove the node pointed to by removeMe from the list whose head node is head.
 * Remove a pointer to the (possibly new) head of the resulting (possibly empty)
 * list.  If free_data_fptr is not NULL, it is a function that will be called
 * to deallocate removeMe->li_data.
 *
 * Note that removeNode() and removeAllNodes() never remove the head node,
 * because they assume that a list always consists of at least one, dataless
 * head node.  By contrast, _evlRemoveNode and _evlFreeList (as well as the
 * other functions in this file) do not assume a dataless head node: the
 * pointer to an empty list is simply NULL.  The latter type of list is
 * prevalent in libevl, the former in some of the utilities.
 */
evl_list_t *
_evlRemoveNode(evl_listnode_t *removeMe, evl_list_t *head,
	void (*free_data_fptr)(void *data))
{
	evl_listnode_t *newHead, *prev, *next;

	assert(removeMe != NULL);
	assert(head != NULL);

	if (free_data_fptr) {
		free_data_fptr((void*) removeMe->li_data);
	}

	if (removeMe == head) {
		/* Removing the head node. */
		newHead = head->li_next;
		if (newHead == head) {
			/* Removing the only node. */
			newHead = NULL;
		}
	} else {
		newHead = head;
	}

	prev = removeMe->li_prev;
	next = removeMe->li_next;
	prev->li_next = next;
	next->li_prev = prev;

	free(removeMe);
	return newHead;
}

/* 
 * Removes all nodes except for the first node, it assumes that
 * the first node is an empty node.
 * It also allows callers to plug in their own data clean up function
 * 
 */
void removeAllNodes(evl_listnode_t *head, void (*free_data_fptr)(void *data))
{
	evl_listnode_t *p;
	evl_listnode_t *prev, *next;
	
	/* 
	 * I assume that the first node is always empty
	 * so I start with the next one
	 */
	p = head->li_next;													
	
	
	if (head == p) {
		/* Point to itself - it is the first empty node, list is empty */
		return;
	}
	
	do {
		next = p->li_next;
		removeNode(p, free_data_fptr);				
		p = next;
	} while(p != head);
}

/* 
 * Removes a node on the list also allows callers to plug in 
 * their own data clean up function
 * 
 */
void removeNode(evl_listnode_t * p, void (* free_data_ptr)(void *data))
{
	evl_listnode_t *next, *prev;
	void * dat;
	dat = (void *)p->li_data;
		
	/*
	 * Adjust the next pointer for the prev node 
	 * and the prev pointer for the next node 
	 */
	prev = p->li_prev;
	next = p->li_next;
	
	prev->li_next = next;
	next->li_prev = prev;
	
	(*free_data_ptr)(dat);
	free(p);
}
