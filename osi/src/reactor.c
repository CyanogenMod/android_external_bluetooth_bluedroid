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

#define LOG_TAG "bt_osi_reactor"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <utils/Log.h>

#include "list.h"
#include "reactor.h"

#if !defined(EFD_SEMAPHORE)
#  define EFD_SEMAPHORE (1 << 0)
#endif

struct reactor_t {
  int event_fd;
  list_t *objects;
};

static reactor_status_t run_reactor(reactor_t *reactor, int iterations, struct timeval *tv);

reactor_t *reactor_new(void) {
  reactor_t *ret = (reactor_t *)calloc(1, sizeof(reactor_t));
  if (!ret)
    return NULL;

  ret->event_fd = eventfd(0, EFD_SEMAPHORE);
  if (ret->event_fd == -1) {
    ALOGE("%s unable to create eventfd: %s", __func__, strerror(errno));
    goto error;
  }

  ret->objects = list_new(NULL);
  if (!ret->objects)
    goto error;

  return ret;

error:;
  list_free(ret->objects);
  close(ret->event_fd);
  free(ret);
  return NULL;
}

void reactor_free(reactor_t *reactor) {
  if (!reactor)
    return;

  list_free(reactor->objects);
  close(reactor->event_fd);
  free(reactor);
}

reactor_status_t reactor_start(reactor_t *reactor) {
  assert(reactor != NULL);
  if(reactor)
     return run_reactor(reactor, 0, NULL);
  else {
     ALOGE("%s :reactor is NULL",__func__);
     return REACTOR_STATUS_ERROR;
  }
}

reactor_status_t reactor_run_once(reactor_t *reactor) {
  assert(reactor != NULL);
  if(reactor)
     return run_reactor(reactor, 1, NULL);
  else {
     ALOGE("%s :reactor is NULL",__func__);
     return REACTOR_STATUS_ERROR;
  }
}

reactor_status_t reactor_run_once_timeout(reactor_t *reactor, timeout_t timeout_ms) {
  assert(reactor != NULL);

  struct timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  if(reactor)
     return run_reactor(reactor, 1, &tv);
  else {
     ALOGE("%s :reactor is NULL",__func__);
     return REACTOR_STATUS_ERROR;
  }
}

void reactor_stop(reactor_t *reactor) {
  assert(reactor != NULL);
  if(reactor)
     eventfd_write(reactor->event_fd, 1);
}

void reactor_register(reactor_t *reactor, reactor_object_t *obj) {
  assert(reactor != NULL);
  assert(obj != NULL);
  if(reactor && obj)
     list_append(reactor->objects, obj);
  else
     ALOGE("%s :reactor or reactor obj is NULL",__func__);
}

void reactor_unregister(reactor_t *reactor, reactor_object_t *obj) {
  assert(reactor != NULL);
  assert(obj != NULL);
  if(!reactor || !obj) {
     ALOGE("%s :reactor or obj is NULL",__func__);
     return;
  }
  list_remove(reactor->objects, obj);
}

// Runs the reactor loop for a maximum of |iterations| with the given timeout, |tv|.
// 0 |iterations| means loop forever.
// NULL |tv| means no timeout (block until an event occurs).
// |reactor| may not be NULL.
static reactor_status_t run_reactor(reactor_t *reactor, int iterations, struct timeval *tv) {
  assert(reactor != NULL);
  if(!reactor) {
     ALOGE("%s :reactor is NULL",__func__);
     return REACTOR_STATUS_ERROR;
  }
  for (int i = 0; iterations == 0 || i < iterations; ++i) {
    fd_set read_set;
    fd_set write_set;
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(reactor->event_fd, &read_set);

    int max_fd = reactor->event_fd;
    for (const list_node_t *iter = list_begin(reactor->objects); iter != list_end(reactor->objects); iter = list_next(iter)) {
      reactor_object_t *object = (reactor_object_t *)list_node(iter);
      int fd = object->fd;
      reactor_interest_t interest = object->interest;
      if (interest & REACTOR_INTEREST_READ)
        FD_SET(fd, &read_set);
      if (interest & REACTOR_INTEREST_WRITE)
        FD_SET(fd, &write_set);
      if (fd > max_fd)
        max_fd = fd;
    }

    int ret;
    do {
      ret = select(max_fd + 1, &read_set, &write_set, NULL, tv);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1) {
      ALOGE("%s error in select: %s", __func__, strerror(errno));
      return REACTOR_STATUS_ERROR;
    }

    if (ret == 0)
      return REACTOR_STATUS_TIMEOUT;

    if (FD_ISSET(reactor->event_fd, &read_set)) {
      eventfd_t value;
      eventfd_read(reactor->event_fd, &value);
      return REACTOR_STATUS_STOP;
    }

    for (const list_node_t *iter = list_begin(reactor->objects); ret > 0 && iter != list_end(reactor->objects); iter = list_next(iter)) {
      reactor_object_t *object = (reactor_object_t *)list_node(iter);
      int fd = object->fd;
      if (FD_ISSET(fd, &read_set)) {
        object->read_ready(object->context);
        --ret;
      }
      if (FD_ISSET(fd, &write_set)) {
        object->write_ready(object->context);
        --ret;
      }
    }
  }
  return REACTOR_STATUS_DONE;
}
