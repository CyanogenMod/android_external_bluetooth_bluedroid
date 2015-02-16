/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
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
#ifndef GKI_INT_H
#define GKI_INT_H

#include "gki_common.h"
#include <pthread.h>
#include <sys/prctl.h>

/**********************************************************************
** OS specific definitions
*/
/* The base priority used for pthread based GKI task. below value is to keep it retro compatible.
 * It is recommended to use (GKI_MAX_TASKS+3), this will assign real time priorities GKI_MAX_TASKS-
 * task_id -2 to the thread */
#ifndef GKI_LINUX_BASE_PRIORITY
#define GKI_LINUX_BASE_PRIORITY 30
#endif

/* The base policy used for pthread based GKI task. the sched defines are defined here to avoid undefined values due
 * to missing header file, see pthread functions! Overall it is recommend however to use SCHED_NOMRAL */
#define GKI_SCHED_NORMAL    0
#define GKI_SCHED_FIFO      1
#define GKI_SCHED_RR        2
#ifndef GKI_LINUX_BASE_POLICY
#define GKI_LINUX_BASE_POLICY GKI_SCHED_NORMAL
#endif

/* GKI timer bases should use GKI_SCHED_FIFO to ensure the least jitter possible */
#ifndef GKI_LINUX_TIMER_POLICY
#define GKI_LINUX_TIMER_POLICY GKI_SCHED_FIFO
#endif

/* the GKI_timer_update() thread should have the highest real time priority to ensue correct
 * timer expiry.
 */
#ifndef GKI_LINUX_TIMER_TICK_PRIORITY
#define GKI_LINUX_TIMER_TICK_PRIORITY GKI_LINUX_BASE_PRIORITY+2
#endif

typedef struct
{
    pthread_mutex_t     GKI_mutex;  /* For buffer */
    pthread_t           thread_id[GKI_MAX_TASKS];
    pthread_mutex_t     thread_evt_mutex[GKI_MAX_TASKS];
    pthread_cond_t      thread_evt_cond[GKI_MAX_TASKS];
    pthread_mutex_t     thread_timeout_mutex[GKI_MAX_TASKS];
    pthread_cond_t      thread_timeout_cond[GKI_MAX_TASKS];
#if (GKI_DEBUG == TRUE)
    pthread_mutex_t     GKI_trace_mutex;
#endif
    pthread_mutex_t     gki_timerupdate_mutex; /* For timer update*/

} tGKI_OS;

extern void gki_system_tick_start_stop_cback(BOOLEAN start);

/* Contains common control block as well as OS specific variables */
typedef struct
{
    tGKI_OS     os;
    tGKI_COM_CB com;
} tGKI_CB;


#ifdef __cplusplus
extern "C" {
#endif

#if GKI_DYNAMIC_MEMORY == FALSE
GKI_API extern tGKI_CB  gki_cb;
#else
GKI_API extern tGKI_CB *gki_cb_ptr;
#define gki_cb (*gki_cb_ptr)
#endif

#ifdef __cplusplus
}
#endif

#endif
