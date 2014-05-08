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

struct thread_t;
typedef struct thread_t thread_t;

typedef void *(*thread_start_cb) (void *);

// Lifecycle
thread_t *thread_create(const char *name,
                         thread_start_cb start_routine, void *arg);
int thread_join(thread_t *thread, void **retval);

// Query
pid_t thread_id(const thread_t *thread);
const char *thread_name(const thread_t *thread);
