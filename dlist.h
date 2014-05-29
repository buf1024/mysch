/* adlist.h - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */

typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
    void *value;
} list_node;

typedef struct list_iter {
    list_node *next;
    int direction;
} list_iter;

typedef struct list {
    list_node *head;
    list_node *tail;/*
    void *(*dup)(void *ptr);
    void (*free)(void *ptr);
    int (*match)(void *ptr, void *key);*/
    unsigned long len;
} list;

/* Functions implemented as macros */
#define list_length(l) ((l)->len)
#define list_first(l) ((l)->head)
#define list_last(l) ((l)->tail)
#define list_prev_node(n) ((n)->prev)
#define list_next_node(n) ((n)->next)
#define list_node_value(n) ((n)->value)

/*
#define list_set_dup_method(l,m) ((l)->dup = (m))
#define list_set_free_method(l,m) ((l)->free = (m))
#define list_set_match_method(l,m) ((l)->match = (m))

#define list_get_dup_method(l) ((l)->dup)
#define list_get_free(l) ((l)->free)
#define list_get_match_method(l) ((l)->match)
*/

/* Prototypes */
list *list_create(void);
void list_release(list *list, int nofree);
list *list_add_node_head(list *list, void *value);
list *list_add_node_tail(list *list, void *value);
list *list_insert_node(list *list, list_node *old_node, void *value, int after);
void list_del_node(list *list, list_node *node, int nofree);
list_iter *list_get_iterator(list *list, int direction);
list_node *list_next(list_iter *iter);
void list_release_iterator(list_iter *iter);
//list *list_dup(list *orig);
list_node *list_search_key(list *list, void *key, int len);
list_node *list_index(list *list, long index);
void list_rewind(list *list, list_iter *li);
void list_rewind_tail(list *list, list_iter *li);
void list_rotate(list *list);

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#define AL_FREE    0
#define AL_NO_FREE 1

#endif /* __ADLIST_H__ */
