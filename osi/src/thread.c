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

#define LOG_TAG "osi_thread"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <utils/Log.h>

#include "fixed_queue.h"
#include "reactor.h"
#include "semaphore.h"
#include "thread.h"

struct thread_t {
  pthread_t pthread;
  pid_t tid;
  char name[THREAD_NAME_MAX + 1];
  reactor_t *reactor;
  fixed_queue_t *work_queue;
};

struct start_arg {
  thread_t *thread;
  semaphore_t *start_sem;
  int error;
};

typedef struct {
  thread_fn func;
  void *context;
} work_item_t;

static void *run_thread(void *start_arg);
static void work_queue_read_cb(void *context);

static const size_t WORK_QUEUE_CAPACITY = 128;

thread_t *thread_new(const char *name) {
  assert(name != NULL);

  if(!name) {
     ALOGE("%s: thread name is NULL", __func__);
     return NULL;
  }
  // Start is on the stack, but we use a semaphore, so it's safe
  thread_t *ret = calloc(1, sizeof(thread_t));
  if (!ret)
    goto error;

  ret->reactor = reactor_new();
  if (!ret->reactor)
    goto error;

  ret->work_queue = fixed_queue_new(WORK_QUEUE_CAPACITY);
  if (!ret->work_queue)
    goto error;

  struct start_arg start;
  start.start_sem = semaphore_new(0);
  if (!start.start_sem)
    goto error;

  strncpy(ret->name, name, THREAD_NAME_MAX);
  start.thread = ret;
  start.error = 0;
  pthread_create(&ret->pthread, NULL, run_thread, &start);
  semaphore_wait(start.start_sem);
  semaphore_free(start.start_sem);
  if (start.error)
    goto error;
  return ret;

error:;
  if (ret) {
    fixed_queue_free(ret->work_queue, free);
    reactor_free(ret->reactor);
  }
  free(ret);
  return NULL;
}

void thread_free(thread_t *thread) {
  if (!thread)
    return;

  thread_stop(thread);
  pthread_join(thread->pthread, NULL);
  fixed_queue_free(thread->work_queue, free);
  reactor_free(thread->reactor);
  free(thread);
}

bool thread_post(thread_t *thread, thread_fn func, void *context) {
  assert(thread != NULL);
  assert(func != NULL);

  if(!thread || !func) {
     ALOGE("%s: thread or func is NULL", __func__);
     return false;
  }
  // TODO(sharvil): if the current thread == |thread| and we've run out
  // of queue space, we should abort this operation, otherwise we'll
  // deadlock.

  // Queue item is freed either when the queue itself is destroyed
  // or when the item is removed from the queue for dispatch.
  work_item_t *item = (work_item_t *)malloc(sizeof(work_item_t));
  if (!item) {
    ALOGE("%s unable to allocate memory: %s", __func__, strerror(errno));
    return false;
  }
  item->func = func;
  item->context = context;
  fixed_queue_enqueue(thread->work_queue, item);
  return true;
}

void thread_stop(thread_t *thread) {
  assert(thread != NULL);
  if(!thread) {
     ALOGE("%s: thread is NULL", __func__);
     return ;
  }
  reactor_stop(thread->reactor);
}

const char *thread_name(const thread_t *thread) {
  assert(thread != NULL);
  if(!thread) {
     ALOGE("%s: thread is NULL", __func__);
     return NULL;
  }
  return thread->name;
}

static void *run_thread(void *start_arg) {
  assert(start_arg != NULL);
  if(!start_arg) {
     ALOGE("%s: arg is NULL", __func__);
     return NULL;
  }
  struct start_arg *start = start_arg;
  thread_t *thread = start->thread;

  assert(thread != NULL);
  if(!thread) {
     ALOGE("%s: thread is NULL", __func__);
     return NULL;
  }

  if (prctl(PR_SET_NAME, (unsigned long)thread->name) == -1) {
    ALOGE("%s unable to set thread name: %s", __func__, strerror(errno));
    start->error = errno;
    semaphore_post(start->start_sem);
    return NULL;
  }
  thread->tid = gettid();

  semaphore_post(start->start_sem);

  reactor_object_t work_queue_object;
  work_queue_object.context = thread->work_queue;
  work_queue_object.fd = fixed_queue_get_dequeue_fd(thread->work_queue);
  work_queue_object.interest = REACTOR_INTEREST_READ;
  work_queue_object.read_ready = work_queue_read_cb;

  reactor_register(thread->reactor, &work_queue_object);
  reactor_start(thread->reactor);

  // Make sure we dispatch all queued work items before exiting the thread.
  // This allows a caller to safely tear down by enqueuing a teardown
  // work item and then joining the thread.
  size_t count = 0;
  work_item_t *item = fixed_queue_try_dequeue(thread->work_queue);
  while (item && count <= WORK_QUEUE_CAPACITY) {
    item->func(item->context);
    free(item);
    item = fixed_queue_try_dequeue(thread->work_queue);
    ++count;
  }

  if (count > WORK_QUEUE_CAPACITY)
    ALOGD("%s growing event queue on shutdown.", __func__);

  return NULL;
}

static void work_queue_read_cb(void *context) {
  assert(context != NULL);
  if(!context) {
     ALOGE("%s: thread context is NULL", __func__);
     return;
  }
  fixed_queue_t *queue = (fixed_queue_t *)context;
  work_item_t *item = fixed_queue_dequeue(queue);
  item->func(item->context);
  free(item);
}
