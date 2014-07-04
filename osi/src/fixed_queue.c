/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "fixed_queue.h"
#include "list.h"
#include "osi.h"
#include "semaphore.h"

typedef struct fixed_queue_t {
  list_t *list;
  semaphore_t *enqueue_sem;
  semaphore_t *dequeue_sem;
  pthread_mutex_t lock;
  size_t capacity;
} fixed_queue_t;

fixed_queue_t *fixed_queue_new(size_t capacity) {
  fixed_queue_t *ret = calloc(1, sizeof(fixed_queue_t));
  if (!ret)
    goto error;

  ret->list = list_new(NULL);
  if (!ret->list)
    goto error;

  ret->enqueue_sem = semaphore_new(capacity);
  if (!ret->enqueue_sem)
    goto error;

  ret->dequeue_sem = semaphore_new(0);
  if (!ret->dequeue_sem)
    goto error;

  pthread_mutex_init(&ret->lock, NULL);
  ret->capacity = capacity;

  return ret;

error:;
  if (ret) {
    list_free(ret->list);
    semaphore_free(ret->enqueue_sem);
    semaphore_free(ret->dequeue_sem);
  }

  free(ret);
  return NULL;
}

void fixed_queue_free(fixed_queue_t *queue, fixed_queue_free_cb free_cb) {
  if (!queue)
    return;

  if (free_cb)
    for (const list_node_t *node = list_begin(queue->list); node != list_end(queue->list); node = list_next(node))
      free_cb(list_node(node));

  list_free(queue->list);
  semaphore_free(queue->enqueue_sem);
  semaphore_free(queue->dequeue_sem);
  pthread_mutex_destroy(&queue->lock);
  free(queue);
}

void fixed_queue_enqueue(fixed_queue_t *queue, void *data) {
  assert(queue != NULL);
  assert(data != NULL);

  semaphore_wait(queue->enqueue_sem);

  pthread_mutex_lock(&queue->lock);
  list_append(queue->list, data);
  pthread_mutex_unlock(&queue->lock);

  semaphore_post(queue->dequeue_sem);
}

void *fixed_queue_dequeue(fixed_queue_t *queue) {
  assert(queue != NULL);

  semaphore_wait(queue->dequeue_sem);

  pthread_mutex_lock(&queue->lock);
  void *ret = list_front(queue->list);
  list_remove(queue->list, ret);
  pthread_mutex_unlock(&queue->lock);

  semaphore_post(queue->enqueue_sem);

  return ret;
}

bool fixed_queue_try_enqueue(fixed_queue_t *queue, void *data) {
  assert(queue != NULL);
  assert(data != NULL);

  if (!semaphore_try_wait(queue->enqueue_sem))
    return false;

  pthread_mutex_lock(&queue->lock);
  list_append(queue->list, data);
  pthread_mutex_unlock(&queue->lock);

  semaphore_post(queue->dequeue_sem);
  return true;
}

void *fixed_queue_try_dequeue(fixed_queue_t *queue) {
  assert(queue != NULL);

  if (!semaphore_try_wait(queue->dequeue_sem))
    return NULL;

  pthread_mutex_lock(&queue->lock);
  void *ret = list_front(queue->list);
  list_remove(queue->list, ret);
  pthread_mutex_unlock(&queue->lock);

  semaphore_post(queue->enqueue_sem);

  return ret;
}

int fixed_queue_get_dequeue_fd(const fixed_queue_t *queue) {
  assert(queue != NULL);
  return semaphore_get_fd(queue->dequeue_sem);
}

int fixed_queue_get_enqueue_fd(const fixed_queue_t *queue) {
  assert(queue != NULL);
  return semaphore_get_fd(queue->enqueue_sem);
}
