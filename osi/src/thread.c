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
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <utils/Log.h>

#include "semaphore.h"
#include "thread.h"

typedef struct thread_t {
  pthread_t pthread;
  pid_t tid;
  char name[THREAD_NAME_MAX+1];
} thread_t;

pid_t thread_id(const thread_t *thread) {
  assert(thread != NULL);
  return thread->tid;
}

const char *thread_name(const thread_t *thread) {
  assert(thread != NULL);
  return thread->name;
}

struct start_arg {
  thread_t *thread;
  semaphore_t *start_sem;
  int error;
  thread_start_cb start_routine;
  void *arg;
};

static void *run_thread(void *start_arg) {
  assert(start_arg != NULL);

  struct start_arg *start = start_arg;
  thread_t *thread = start->thread;

  assert(thread != NULL);

  if (prctl(PR_SET_NAME, (unsigned long)thread->name) == -1) {
    ALOGE("%s unable to set thread name: %s", __func__, strerror(errno));
    start->error = errno;
    semaphore_post(start->start_sem);
    return NULL;
  }
  thread->tid = gettid();

  // Cache local values because we are about to let thread_create
  // continue
  thread_start_cb start_routine = start->start_routine;
  void *arg = start->arg;

  semaphore_post(start->start_sem);
  return start_routine(arg);
}

thread_t *thread_create(const char *name,
                        thread_start_cb start_routine, void *arg) {
  assert(name != NULL);
  assert(start_routine != NULL);

  // Start is on the stack, but we use a semaphore, so it's safe
  struct start_arg start;
  thread_t *ret;
  ret = calloc(1, sizeof(thread_t));
  if (!ret)
    goto error;
  start.start_sem = semaphore_new(0);
  if (!start.start_sem)
    goto error;

  strncpy(ret->name, name, THREAD_NAME_MAX);
  start.thread = ret;
  start.error = 0;
  start.start_routine = start_routine;
  start.arg = arg;
  pthread_create(&ret->pthread, NULL, run_thread, &start);
  semaphore_wait(start.start_sem);
  if (start.error)
    goto error;
  return ret;

error:;
  semaphore_free(start.start_sem);
  free(ret);
  return NULL;
}

int thread_join(thread_t *thread, void **retval) {
  int ret = pthread_join(thread->pthread, retval);
  if (!ret)
    free(thread);
  return ret;
}
