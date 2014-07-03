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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "osi.h"

// This module implements the Reactor pattern.
// See http://en.wikipedia.org/wiki/Reactor_pattern for details.

struct reactor_t;
typedef struct reactor_t reactor_t;

struct reactor_object_t;
typedef struct reactor_object_t reactor_object_t;

// Enumerates the types of events a reactor object is interested
// in responding to.
typedef enum {
  REACTOR_INTEREST_READ  = 1,
  REACTOR_INTEREST_WRITE = 2,
  REACTOR_INTEREST_READ_WRITE = 3,
} reactor_interest_t;

// Enumerates the reasons a reactor has stopped.
typedef enum {
  REACTOR_STATUS_STOP,     // |reactor_stop| was called.
  REACTOR_STATUS_TIMEOUT,  // a timeout was specified and the reactor timed out.
  REACTOR_STATUS_ERROR,    // there was an error during the operation.
  REACTOR_STATUS_DONE,     // the reactor completed its work (for the _run_once* variants).
} reactor_status_t;

struct reactor_object_t {
  void *context;                       // a context that's passed back to the *_ready functions.
  int fd;                              // the file descriptor to monitor for events.
  reactor_interest_t interest;         // the event types to monitor the file descriptor for.

  void (*read_ready)(void *context);   // function to call when the file descriptor becomes readable.
  void (*write_ready)(void *context);  // function to call when the file descriptor becomes writeable.
};

// Creates a new reactor object. Returns NULL on failure. The returned object
// must be freed by calling |reactor_free|.
reactor_t *reactor_new(void);

// Frees a reactor object created with |reactor_new|. |reactor| may be NULL.
void reactor_free(reactor_t *reactor);

// Starts the reactor. This function blocks the caller until |reactor_stop| is called
// from another thread or in a callback. |reactor| may not be NULL.
reactor_status_t reactor_start(reactor_t *reactor);

// Runs one iteration of the reactor. This function blocks until at least one registered object
// becomes ready. |reactor| may not be NULL.
reactor_status_t reactor_run_once(reactor_t *reactor);

// Same as |reactor_run_once| with a bounded wait time in case no object becomes ready.
reactor_status_t reactor_run_once_timeout(reactor_t *reactor, timeout_t timeout_ms);

// Immediately unblocks the reactor. This function is safe to call from any thread.
// |reactor| may not be NULL.
void reactor_stop(reactor_t *reactor);

// Registers an object with the reactor. |obj| is neither copied nor is its ownership transferred
// so the pointer must remain valid until it is unregistered with |reactor_unregister|. Neither
// |reactor| nor |obj| may be NULL.
void reactor_register(reactor_t *reactor, reactor_object_t *obj);

// Unregisters a previously registered object with the |reactor|. Neither |reactor| nor |obj|
// may be NULL.
void reactor_unregister(reactor_t *reactor, reactor_object_t *obj);
