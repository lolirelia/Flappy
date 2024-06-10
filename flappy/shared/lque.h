/*

Github: https://github.com/lag13/CGenericQueue/blob/master/lqueue.h
A generic queue datatype in C using macros.
*/
#ifndef _L_QUEUE_H_
#define _L_QUEUE_H_

#include <stdlib.h>

/*
Creates a named struct which represents the queue datatype.
This would be used if you wanted to write functions that
accepted these queue types.
*/
#define LQUEUE_CREATE_DATATYPE(name, type) \
    struct name {                          \
        struct {                           \
            type data;                     \
            void *next;                    \
        } *head, *tail, *misc;             \
        size_t size;                       \
    }

/*
Used to declare a variable of type queue by creating an
anonymous struct representing a queue.
*/
#define lqueue_t(type)         \
    struct {                   \
        struct {               \
            type data;         \
            void *next;        \
        } *head, *tail, *misc; \
        size_t size;           \
    }

/*
Initializes a queue.
*/
#define lqueue_init(q) ((q).head = NULL, (q).size = 0)

/*
Frees up the memory occupied by this queue.
*/
#define lqueue_del(q)                      \
    do {                                   \
        if ((q).size > 0) {                \
            (q).misc = (q).head;           \
            (q).tail = (q).head->next;     \
            free((q).head);                \
            (q).head = (q).tail;           \
            while ((q).head != (q).misc) { \
                (q).tail = (q).head->next; \
                free((q).head);            \
                (q).head = (q).tail;       \
            }                              \
        }                                  \
    } while (0)

/*
Append an item to the queue.
*/
#define lqueue_append(x, q)                                   \
    do {                                                      \
        if ((q).head == NULL) {                               \
            (q).head = malloc(sizeof(*(q).head));             \
            (q).tail = (q).head;                              \
            (q).tail->next = (q).head;                        \
        } else {                                              \
            if ((q).tail->next == (q).head && (q).size > 0) { \
                (q).tail->next = malloc(sizeof(*(q).head));   \
                (q).tail = (q).tail->next;                    \
                (q).tail->next = (q).head;                    \
            } else {                                          \
                (q).tail = (q).tail->next;                    \
            }                                                 \
        }                                                     \
        (q).tail->data = x;                                   \
        (q).size++;                                           \
    } while (0)

/*
Removes an item from the front of the queue.
*/
#define lqueue_remove(q) \
    ((q).misc = (q).head, (q).head = (q).head->next, (q).size--, (q).misc->data)

/*
Looks at the first element in the queue.
*/
#define lqueue_peek(q) ((q).head->data)

/*
Returns true or false if the queue is empty or not.
*/
#define lqueue_empty(q) ((q).size == 0)

/*
Returns the number of elements in this queue.
*/
#define lqueue_size(q) ((q).size)

/*
Applies a function to each element of the queue.
*/
#define lqueue_foreach(func, q)                  \
    do {                                         \
        if ((q).size > 0) {                      \
            (q).misc = (q).head;                 \
            func((q).misc->data);                \
            (q).misc = (q).misc->next;           \
            while ((q).misc != (q).tail->next) { \
                func((q).misc->data);            \
                (q).misc = (q).misc->next;       \
            }                                    \
        }                                        \
    } while (0)

#endif