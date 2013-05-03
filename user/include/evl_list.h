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

#ifndef _EVL_LIST_H_
#define _EVL_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

struct evl_listnode {
	struct evl_listnode	*li_next, *li_prev;
	const void		*li_data;
};
typedef struct evl_listnode evl_list_t;
typedef struct evl_listnode evl_listnode_t;

extern evl_listnode_t *_evlAllocListNode();
extern evl_listnode_t *_evlMkListNode(const void *data);
extern evl_list_t *_evlAppendToList(evl_list_t *head, const void *data);
extern int _evlGetListSize(const evl_list_t *head);
extern evl_listnode_t *_evlFindNamedItemInList(const evl_list_t *head,
	const char *name);
extern void _evlFreeList(evl_list_t *head, int freeData);
extern evl_listnode_t *_evlGetNthNode(evl_list_t *head, int n);
extern void *_evlGetNthValue(evl_list_t *head, int n);
extern void _evlInsertListNode(evl_listnode_t *insertMe,
	evl_listnode_t *beforeMe);
extern evl_list_t *_evlInsertToList(void *insertMe, evl_listnode_t *beforeMe,
	evl_list_t *list);
extern evl_list_t *_evlRemoveNode(evl_listnode_t *removeMe, evl_list_t *head,
        void (*free_data_fptr)(void *data));
extern void removeNode(evl_listnode_t * p, void (* free_data_fptr)(void *data));
extern void removeAllNodes(evl_listnode_t *head, void (*free_data_fptr)(void *data));

#ifdef __cplusplus
}
#endif

#endif /* _EVL_LIST_H_ */
