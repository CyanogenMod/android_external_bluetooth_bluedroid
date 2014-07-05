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

#define THREAD_NAME_MAX 16

typedef struct thread_t thread_t;
typedef void (*thread_fn)(void *context);

// Creates and starts a new thread with the given name. Only THREAD_NAME_MAX
// bytes from |name| will be assigned to the newly-created thread. Returns a
// thread object if the thread was successfully started, NULL otherwise. The
// returned thread object must be freed with |thread_free|. |name| may not
// be NULL.
thread_t *thread_new(const char *name);

// Frees the given |thread|. If the thread is still running, it is stopped
// and the calling thread will block until |thread| terminates. |thread|
// may be NULL.
void thread_free(thread_t *thread);

// Call |func| with the argument |context| on |thread|. This function typically
// does not block unless there are an excessive number of functions posted to
// |thread| that have not been dispatched yet. Neither |thread| nor |func| may
// be NULL. |context| may be NULL.
bool thread_post(thread_t *thread, thread_fn func, void *context);

// Requests |thread| to stop. Only |thread_free| and |thread_name| may be called
// after calling |thread_stop|. This function is guaranteed to not block.
// |thread| may not be NULL.
void thread_stop(thread_t *thread);

// Returns the name of the given |thread|. |thread| may not be NULL.
const char *thread_name(const thread_t *thread);
