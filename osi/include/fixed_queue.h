#pragma once

#include "list.h"

struct fixed_queue_t;
typedef struct fixed_queue_t fixed_queue_t;

typedef void (*fixed_queue_free_cb)(void *data);

fixed_queue_t *fixed_queue_new(size_t capacity);

// Freeing a queue that is currently in use (i.e. has waiters
// blocked on it) resuls in undefined behaviour.
void fixed_queue_free(fixed_queue_t *queue, fixed_queue_free_cb free_cb);

void fixed_queue_enqueue(fixed_queue_t *queue, void *data);
void *fixed_queue_dequeue(fixed_queue_t *queue);
