/*
 * myqueue.h
 *
 *  Created on: 2014-1-19
 *      Author: Luo Guochun
 */

#ifndef __MYQUEUE_H__
#define __MYQUEUE_H__

#define __OFFSET(type, field)                         \
    ((size_t)&(((type*)(0))->field))

#define __CONTAINEROF(elm, type, val) \
    (type*)(const volatile void*) \
    ((const volatile char*)(elm) - __OFFSET(type, val))

typedef void (*fn_free_t)(void*);

/*
 * Singly-linked List declarations.
 */
typedef struct slist_node_s slist_node_t;
typedef struct slist_s slist_t;

struct slist_node_s
{
    struct slist_node_s* sle_next; /* next element */
    void* data; /* data */
    fn_free_t fn_free; /* free data fun*/
};

struct slist_s
{
    struct slist_node_s* slh_first; /* first element */
};
/*
 * Singly-linked List functions.
 */
slist_t* slist_create();
void slist_destroy(slist_t* list);

slist_node_t* slist_node_create(void* data, fn_free_t fn_free);
void slist_node_destroy(slist_node_t* node);

#define slist_node_data(node) ((node)->data)
#define slist_set_node_data(node, d) ((node)->data = (d))

#define slist_init(head) do{slist_first(head) = NULL;}while(0)
#define slist_empty(head)   ((head)->slh_first == NULL)
#define slist_first(head)   ((head)->slh_first)
#define slist_next(elm)  ((elm)->sle_next)

#define slist_insert_after(slistelm, elm) do {           \
        slist_next((elm)) = slist_next((slistelm));   \
        slist_next((slistelm)) = (elm);              \
} while (0)

#define slist_insert_head(head, elm) do {            \
        slist_next((elm)) = slist_first((head));         \
        slist_first((head)) = (elm);                    \
} while (0)

#define slist_remove_head(head) do {             \
        slist_first((head)) = slist_next(slist_first((head)));   \
} while (0)

#define slist_remove_after(elm) do {             \
        slist_next(elm) =                    \
        slist_next(slist_next(elm));          \
} while (0)


#define slist_remove(head, elm) do {           \
    if (slist_first((head)) == (elm)) {             \
        slist_remove_head((head));           \
    }                               \
    else {                              \
        struct slist_node_s *curelm = slist_first((head));      \
        while (slist_next(curelm) != (elm))      \
            curelm = slist_next(curelm);     \
            slist_remove_after(curelm);          \
    }                               \
} while (0)

#define slist_swap(head1, head2) do {             \
    struct slist_node_s *swap_first = slist_first(head1);           \
    slist_first(head1) = slist_first(head2);            \
    slist_first(head2) = swap_first;                \
} while (0)

/*
 * Singly-linked Tail queue declarations.
 */
typedef struct stailq_node_s stailq_node_t;
typedef struct stailq_s stailq_t;

struct stailq_node_s
{
    struct stailq_node_s* stqe_next; /* next element*/
    void* data; /* data */
    fn_free_t fn_free; /* free data */
};

struct stailq_s
{
    struct stailq_node_s *stqh_first;/* first element */         \
    struct stailq_node_s **stqh_last;/* addr of last next element */     \
};

/*
 * Singly-linked Tail queue functions.
 */

stailq_t* stailq_create();
void stailq_destroy(stailq_t* stailq);

stailq_node_t* stailq_node_create(void* data, fn_free_t fn_free);
void stailq_node_destroy(stailq_node_t* node);

#define stailq_node_data(node) ((node)->data)
#define stailq_set_node_data(node, d) ((node)->data = (d))

#define stailq_init(head) do {                      \
        stailq_first((head)) = NULL;                    \
        (head)->stqh_last = &stailq_first((head));          \
} while (0)
#define stailq_empty(head)  ((head)->stqh_first == NULL)
#define stailq_first(head)  ((head)->stqh_first)
#define stailq_next(elm) ((elm)->stqe_next)
#define stailq_last(head)                  \
    (stailq_empty((head)) ? NULL :                  \
        __CONTAINEROF((head)->stqh_last, struct stailq_node_s, stqe_next))

#define stailq_insert_after(head, tqelm, elm) do {       \
    if ((stailq_next((elm)) = stailq_next((tqelm))) == NULL)\
        (head)->stqh_last = &stailq_next((elm));     \
    stailq_next((tqelm)) = (elm);                \
} while (0)

#define stailq_insert_head(head, elm) do {           \
    if ((stailq_next((elm)) = stailq_first((head))) == NULL) \
        (head)->stqh_last = &stailq_next((elm));     \
    stailq_first((head)) = (elm);                   \
} while (0)

#define stailq_insert_tail(head, elm) do {           \
    stailq_next((elm)) = NULL;               \
    *(head)->stqh_last = (elm);                 \
    (head)->stqh_last = &stailq_next((elm));         \
} while (0)

#define stailq_remove(head, elm) do {          \
    if (stailq_first((head)) == (elm)) {                \
        stailq_remove_head((head));          \
    }                               \
    else {                              \
        struct stailq_node_s *curelm = stailq_first((head));     \
        while (stailq_next(curelm) != (elm))     \
            curelm = stailq_next(curelm);        \
        stailq_remove_after(head, curelm);       \
    }                               \
} while (0)

#define stailq_remove_after(head, elm) do {          \
    if ((stailq_next(elm) =                  \
         stailq_next(stailq_next(elm))) == NULL)  \
        (head)->stqh_last = &stailq_next((elm));     \
} while (0)

#define stailq_remove_head(head) do {                \
    if ((stailq_first((head)) =                 \
         stailq_next(stailq_first((head)))) == NULL)     \
        (head)->stqh_last = &stailq_first((head));      \
} while (0)

#define stailq_swap(head1, head2) do {                \
    struct stailq_node_s *swap_first = stailq_first(head1);          \
    struct stailq_node_s **swap_last = (head1)->stqh_last;           \
    stailq_first(head1) = stailq_first(head2);          \
    (head1)->stqh_last = (head2)->stqh_last;            \
    stailq_first(head2) = swap_first;               \
    (head2)->stqh_last = swap_last;                 \
    if (stailq_empty(head1))                    \
        (head1)->stqh_last = &stailq_first(head1);      \
    if (stailq_empty(head2))                    \
        (head2)->stqh_last = &stailq_first(head2);      \
} while (0)

/*
 * List declarations.
 */

typedef struct list_node_s list_node_t;
typedef struct list_s list_t;

struct list_node_s
{
    struct list_node_s *le_next;   /* next element */
    struct list_node_s **le_prev;  /* address of previous next element */
    void* data; /* data */
    fn_free_t fn_free; /* free data */
};

struct list_s
{
    struct list_node_s *lh_first;/* first element */
};

/*
 * List functions.
 */

list_t* list_create();
void list_destroy(list_t* list);

list_node_t* list_node_create(void* data, fn_free_t fn_free);
void list_node_destroy(list_node_t* node);

#define list_node_data(node) ((node)->data)
#define list_set_node_data(node, d) ((node)->data = (d))

#define list_init(head) do { list_first((head)) = NULL; } while (0)
#define list_empty(head)    ((head)->lh_first == NULL)
#define list_first(head)    ((head)->lh_first)
#define list_next(elm)   ((elm)->le_next)
#define list_prev(elm, head)               \
    ((elm)->le_prev == &list_first((head)) ? NULL :       \
            __CONTAINEROF((elm)->le_prev, struct list_node_s, le_next))

#define list_insert_after(listelm, elm) do {         \
    if ((list_next((elm)) = list_next((listelm))) != NULL)\
        list_next((listelm))->le_prev =        \
            &list_next((elm));               \
    list_next((listelm)) = (elm);                \
    (elm)->le_prev = &list_next((listelm));        \
} while (0)

#define list_insert_before(listelm, elm) do {            \
    (elm)->le_prev = (listelm)->le_prev;        \
    list_next((elm)) = (listelm);                \
    *(listelm)->le_prev = (elm);              \
    (listelm)->le_prev = &list_next((elm));        \
} while (0)

#define list_insert_head(head, elm) do {             \
    if ((list_next((elm)) = list_first((head))) != NULL) \
        list_first((head))->le_prev = &list_next((elm));\
    list_first((head)) = (elm);                 \
    (elm)->le_prev = &list_first((head));         \
} while (0)

#define list_remove(elm) do {                    \
    if (list_next((elm)) != NULL)                \
        list_next((elm))->le_prev =        \
            (elm)->le_prev;               \
    *(elm)->le_prev = list_next((elm));        \
} while (0)

#define list_swap(head1, head2) do {           \
    struct list_node_s *swap_tmp = list_first((head1));            \
    list_first((head1)) = list_first((head2));          \
    list_first((head2)) = swap_tmp;                 \
    if ((swap_tmp = list_first((head1))) != NULL)           \
        swap_tmp->le_prev = &list_first((head1));     \
    if ((swap_tmp = list_first((head2))) != NULL)           \
        swap_tmp->le_prev = &list_first((head2));     \
} while (0)

/*
 * Tail queue declarations.
 */

typedef struct tailq_node_s tailq_node_t;
typedef struct tailq_s tailq_t;

struct tailq_node_s
{
    struct tailq_node_s *tqe_next;   /* next element */
    struct tailq_node_s **tqe_prev;  /* address of previous next element */
    void* data; /* data */
    fn_free_t fn_free; /* free data */
};

struct tailq_s
{
    struct tailq_node_s *tqh_first;/* first element */
    struct tailq_node_s **tqh_last; /* addr of last next element */
};
/*
 * Tail queue functions.
 */
tailq_t* tailq_create();
void tailq_destroy(tailq_t* tailq);

tailq_node_t* tailq_node_create(void* data, fn_free_t fn_free);
void tailq_node_destroy(tailq_node_t* node);

#define tailq_node_data(node) ((node)->data)
#define tailq_set_node_data(node, d) ((node)->data = (d))

#define tailq_init(head) do {                  \
    (head)->tqh_last = &tailq_first((head));            \
} while (0)

#define tailq_empty(head)   ((head)->tqh_first == NULL)
#define tailq_first(head)   ((head)->tqh_first)
#define tailq_last(head) (*(((struct tailq_s *)((head)->tqh_last))->tqh_last))
#define tailq_next(elm) ((elm)->tqe_next)
#define tailq_prev(elm) (*(((struct tailq_node_s *)((elm)->tqe_prev))->tqh_last))

#define tailq_concat(head1, head2) do {              \
    if (!tailq_empty(head2)) {                  \
        *(head1)->tqh_last = (head2)->tqh_first;        \
        (head2)->tqh_first->tqe_prev = (head1)->tqh_last; \
        (head1)->tqh_last = (head2)->tqh_last;          \
        tailq_init((head2));                    \
    }                               \
} while (0)

#define tailq_insert_after(head, listelm, elm) do {      \
    if ((tailq_next((elm)) = tailq_next((listelm))) != NULL)\
        tailq_next((elm), field)->tqe_prev =      \
            &tailq_next((elm));              \
    else {                              \
        (head)->tqh_last = &tailq_next((elm));       \
    }                               \
    tailq_next((listelm)) = (elm);               \
    (elm)->tqe_prev = &tailq_next((listelm));      \
} while (0)

#define tailq_insert_before(listelm, elm) do {           \
    (elm)->tqe_prev = (listelm)->tqe_prev;      \
    tailq_next((elm)) = (listelm);               \
    *(listelm)->tqe_prev = (elm);             \
    (listelm)->tqe_prev = &tailq_next((elm));      \
} while (0)

#define tailq_insert_head(head, elm) do {            \
    if ((tailq_next((elm)) = tailq_first((head))) != NULL)   \
        tailq_first((head))->tqe_prev =           \
            &tailq_next((elm));              \
    else                                \
        (head)->tqh_last = &tailq_next((elm));       \
    tailq_first((head)) = (elm);                    \
    (elm)->tqe_prev = &tailq_first((head));           \
} while (0)

#define tailq_insert_tail(head, elm) do {            \
    tailq_next((elm)) = NULL;                \
    (elm)->tqe_prev = (head)->tqh_last;           \
    *(head)->tqh_last = (elm);                  \
    (head)->tqh_last = &tailq_next((elm));           \
} while (0)

#define tailq_remove(head, elm) do {             \
    if ((tailq_next((elm))) != NULL)             \
        tailq_next((elm))->tqe_prev =      \
            (elm)->tqe_prev;              \
    else {                              \
        (head)->tqh_last = (elm)->tqe_prev;       \
    }                               \
    *(elm)->tqe_prev = tailq_next((elm));      \
} while (0)

#define tailq_swap(head1, head2) do {          \
    struct tailq_node_s *swap_first = (head1)->tqh_first;           \
    struct tailq_node_s **swap_last = (head1)->tqh_last;            \
    (head1)->tqh_first = (head2)->tqh_first;            \
    (head1)->tqh_last = (head2)->tqh_last;              \
    (head2)->tqh_first = swap_first;                \
    (head2)->tqh_last = swap_last;                  \
    if ((swap_first = (head1)->tqh_first) != NULL)          \
        swap_first->tqe_prev = &(head1)->tqh_first;   \
    else                                \
        (head1)->tqh_last = &(head1)->tqh_first;        \
    if ((swap_first = (head2)->tqh_first) != NULL)          \
        swap_first->tqe_prev = &(head2)->tqh_first;   \
    else                                \
        (head2)->tqh_last = &(head2)->tqh_first;        \
} while (0)

#endif /* __MYQUEUE_H__ */
