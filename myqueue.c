/*
 * myqueue.c
 *
 *  Created on: 2014-1-19
 *      Author: Luo Guochun
 */

#include <stdlib.h>
#include "myqueue.h"

#define ZERO_MEMORY(item) do { \
    size_t i = 0; \
    for(i =0; i<sizeof(*item); i++) { \
        ((char*)(item))[i] = 0; \
    }\
}while(0)

#define MY_QUEUE_CREATE(type, var) do { \
    var = (type##_t*)malloc(sizeof(*var)); \
    if(var) { \
        type##_init(var); \
    } \
}while(0)

#define MY_QUEUE_DESTROY(type, var) do { \
    if(var) { \
        type##_node_t* node = NULL; \
        for(node = type##_first(var); node; node = type##_next(node)) { \
            type##_node_destroy(node); \
        } \
    } \
}while(0)

#define MY_QUEUE_NODE_CREATE(type, node, data, fun_free) do { \
    node = (type##_node_t*) malloc(sizeof(*node)); \
    if (node) { \
        ZERO_MEMORY(node); \
        node->data = data; \
        node->fn_free = fn_free; \
    } \
}while(0)

#define MY_QUEUE_NODE_DESTRORY(node) do { \
    if((node)) { \
        if((node)->fn_free) { \
            (node)->fn_free((node)->data); \
        } \
        free(node); \
    } \
}while(0)

slist_t* slist_create()
{
    slist_t* list = NULL;

    MY_QUEUE_CREATE(slist, list);

    return list;
}
void slist_destroy(slist_t* list)
{
    MY_QUEUE_DESTROY(slist, list);
}

slist_node_t* slist_node_create(void* data, fn_free_t fn_free)
{
    slist_node_t* node = NULL;

    MY_QUEUE_NODE_CREATE(slist, node, data, fn_free);

    return node;
}
void slist_node_destroy(slist_node_t* node)
{
    MY_QUEUE_NODE_DESTRORY(node);
}


stailq_t* stailq_create()
{
    stailq_t* stailq = NULL;

    MY_QUEUE_CREATE(stailq, stailq);

    return stailq;
}
void stailq_destroy(stailq_t* stailq)
{
    MY_QUEUE_DESTROY(stailq, stailq);
}

stailq_node_t* stailq_node_create(void* data, fn_free_t fn_free)
{
    stailq_node_t* node = NULL;

    MY_QUEUE_NODE_CREATE(stailq, node, data, fn_free);

    return node;
}
void stailq_node_destroy(stailq_node_t* node)
{
    MY_QUEUE_NODE_DESTRORY(node);
}

list_t* list_create()
{
    list_t* list = NULL;

    MY_QUEUE_CREATE(list, list);

    return list;
}
void list_destroy(list_t* list)
{
    MY_QUEUE_DESTROY(list, list);
}

list_node_t* list_node_create(void* data, fn_free_t fn_free)
{
    list_node_t* node = NULL;

    MY_QUEUE_NODE_CREATE(list, node, data, fn_free);

    return node;
}
void list_node_destroy(list_node_t* node)
{
    MY_QUEUE_NODE_DESTRORY(node);
}

tailq_t* tailq_create()
{
    tailq_t* tailq = NULL;

    MY_QUEUE_CREATE(tailq, tailq);

    return tailq;
}
void tailq_destroy(tailq_t* tailq)
{
    MY_QUEUE_DESTROY(tailq, tailq);
}

tailq_node_t* tailq_node_create(void* data, fn_free_t fn_free)
{
    tailq_node_t* node = NULL;

    MY_QUEUE_NODE_CREATE(tailq, node, data, fn_free);

    return node;
}
void tailq_node_destroy(tailq_node_t* node)
{
    MY_QUEUE_NODE_DESTRORY(node);
}
