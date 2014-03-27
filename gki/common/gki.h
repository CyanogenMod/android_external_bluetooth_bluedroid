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
#ifndef GKI_H
#define GKI_H


#include "bt_target.h"
#include "bt_types.h"

#if (defined(OBX_OVER_L2CAP_INCLUDED) && OBX_OVER_L2CAP_INCLUDED == TRUE)
#ifndef GKI_POOL_ID_10
#define GKI_POOL_ID_10             10
#endif

#ifndef GKI_BUF10_SIZE
#define GKI_BUF10_SIZE            (65000 + 24)
#endif

#ifndef GKI_BUF10_MAX
#define GKI_BUF10_MAX           16
#endif

#else

#ifndef GKI_POOL_ID_10
#define GKI_POOL_ID_10                0
#endif /* ifndef GKI_POOL_ID_10 */

#ifndef GKI_BUF10_SIZE
#define GKI_BUF10_SIZE                0
#endif /* ifndef GKI_BUF10_SIZE */

#ifndef GKI_BUF10_MAX
#define GKI_BUF10_MAX           0
#endif

#endif


/* Error codes */
#define GKI_SUCCESS         0x00
#define GKI_FAILURE         0x01
#define GKI_INVALID_TASK    0xF0
#define GKI_INVALID_POOL    0xFF


/************************************************************************
** Mailbox definitions. Each task has 4 mailboxes that are used to
** send buffers to the task.
*/
#define TASK_MBOX_0    0
#define TASK_MBOX_1    1
#define TASK_MBOX_2    2
#define TASK_MBOX_3    3

#define NUM_TASK_MBOX  4

/************************************************************************
** Event definitions.
**
** There are 4 reserved events used to signal messages rcvd in task mailboxes.
** There are 4 reserved events used to signal timeout events.
** There are 8 general purpose events available for applications.
*/
#define MAX_EVENTS              16

#define TASK_MBOX_0_EVT_MASK   0x0001
#define TASK_MBOX_1_EVT_MASK   0x0002
#define TASK_MBOX_2_EVT_MASK   0x0004
#define TASK_MBOX_3_EVT_MASK   0x0008


#define TIMER_0             0
#define TIMER_1             1
#define TIMER_2             2
#define TIMER_3             3

#define TIMER_0_EVT_MASK    0x0010
#define TIMER_1_EVT_MASK    0x0020
#define TIMER_2_EVT_MASK    0x0040
#define TIMER_3_EVT_MASK    0x0080

#define APPL_EVT_0          8
#define APPL_EVT_1          9
#define APPL_EVT_2          10
#define APPL_EVT_3          11
#define APPL_EVT_4          12
#define APPL_EVT_5          13
#define APPL_EVT_6          14
#define APPL_EVT_7          15

#define EVENT_MASK(evt)       ((UINT16)(0x0001 << (evt)))

/* Timer list entry callback type
*/
typedef void (TIMER_CBACK)(void *p_tle);
#ifndef TIMER_PARAM_TYPE
#define TIMER_PARAM_TYPE    UINT32
#endif
/* Define a timer list entry
*/
typedef struct _tle
{
    struct _tle  *p_next;
    struct _tle  *p_prev;
    TIMER_CBACK  *p_cback;
    INT32         ticks;
    INT32         ticks_initial;
    TIMER_PARAM_TYPE   param;
    TIMER_PARAM_TYPE   data;
    UINT16        event;
    UINT8         in_use;
} TIMER_LIST_ENT;

/* Define a timer list queue
*/
typedef struct
{
    TIMER_LIST_ENT   *p_first;
    TIMER_LIST_ENT   *p_last;
} TIMER_LIST_Q;


/***********************************************************************
** This queue is a general purpose buffer queue, for application use.
*/
typedef struct
{
    void    *p_first;
    void    *p_last;
    UINT16   count;
} BUFFER_Q;

#define GKI_IS_QUEUE_EMPTY(p_q) ((p_q)->count == 0)

/* Task constants
*/
#ifndef TASKPTR
typedef void (*TASKPTR)(UINT32);
#endif


#define GKI_PUBLIC_POOL         0       /* General pool accessible to GKI_getbuf() */
#define GKI_RESTRICTED_POOL     1       /* Inaccessible pool to GKI_getbuf() */

/***********************************************************************
** Function prototypes
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Task management
*/
GKI_API extern UINT8   GKI_create_task (TASKPTR, UINT8, INT8 *, UINT16 *, UINT16);
GKI_API extern void    GKI_destroy_task(UINT8 task_id);
GKI_API extern void    GKI_task_self_cleanup(UINT8 task_id);
GKI_API extern void    GKI_exit_task(UINT8);
GKI_API extern UINT8   GKI_get_taskid(void);
GKI_API extern void    GKI_init(void);
GKI_API extern void    GKI_shutdown(void);
GKI_API extern INT8   *GKI_map_taskname(UINT8);
GKI_API extern void    GKI_run(void);
GKI_API extern void    GKI_stop(void);

/* To send buffers and events between tasks
*/
GKI_API extern void   *GKI_read_mbox  (UINT8);
GKI_API extern void    GKI_send_msg   (UINT8, UINT8, void *);
GKI_API extern UINT8   GKI_send_event (UINT8, UINT16);


/* To get and release buffers, change owner and get size
*/
GKI_API extern void    GKI_freebuf (void *);
GKI_API extern void   *GKI_getbuf (UINT16);
GKI_API extern UINT16  GKI_get_buf_size (void *);
GKI_API extern void   *GKI_getpoolbuf (UINT8);
GKI_API extern UINT16  GKI_poolcount (UINT8);
GKI_API extern UINT16  GKI_poolfreecount (UINT8);
GKI_API extern UINT16  GKI_poolutilization (UINT8);


/* User buffer queue management
*/
GKI_API extern void   *GKI_dequeue  (BUFFER_Q *);
GKI_API extern void    GKI_enqueue (BUFFER_Q *, void *);
GKI_API extern void    GKI_enqueue_head (BUFFER_Q *, void *);
GKI_API extern void   *GKI_getfirst (BUFFER_Q *);
GKI_API extern void   *GKI_getlast (BUFFER_Q *);
GKI_API extern void   *GKI_getnext (void *);
GKI_API extern void    GKI_init_q (BUFFER_Q *);
GKI_API extern BOOLEAN GKI_queue_is_empty(BUFFER_Q *);
GKI_API extern void   *GKI_remove_from_queue (BUFFER_Q *, void *);
GKI_API extern UINT16  GKI_get_pool_bufsize (UINT8);

/* Timer management
*/
GKI_API extern void    GKI_add_to_timer_list (TIMER_LIST_Q *, TIMER_LIST_ENT  *);
GKI_API extern void    GKI_delay(UINT32);
GKI_API extern UINT32  GKI_get_tick_count(void);
GKI_API extern void    GKI_init_timer_list (TIMER_LIST_Q *);
GKI_API extern INT32   GKI_ready_to_sleep (void);
GKI_API extern BOOLEAN GKI_remove_from_timer_list (TIMER_LIST_Q *, TIMER_LIST_ENT  *);
GKI_API extern void    GKI_start_timer(UINT8, INT32, BOOLEAN);
GKI_API extern void    GKI_stop_timer (UINT8);
GKI_API extern void    GKI_timer_update(INT32);
GKI_API extern UINT16  GKI_update_timer_list (TIMER_LIST_Q *, INT32);
GKI_API extern UINT32  GKI_get_remaining_ticks (TIMER_LIST_Q *, TIMER_LIST_ENT  *);
GKI_API extern UINT16  GKI_wait(UINT16, UINT32);
GKI_API extern BOOLEAN GKI_timer_queue_is_empty(const TIMER_LIST_Q *timer_q);
GKI_API extern TIMER_LIST_ENT *GKI_timer_getfirst(const TIMER_LIST_Q *timer_q);
GKI_API extern INT32 GKI_timer_ticks_getinitial(const TIMER_LIST_ENT *tle);

/* Disable Interrupts, Enable Interrupts
*/
GKI_API extern void    GKI_enable(void);
GKI_API extern void    GKI_disable(void);

/* Allocate (Free) memory from an OS
*/
GKI_API extern void     *GKI_os_malloc (UINT32);
GKI_API extern void      GKI_os_free (void *);

/* os timer operation */
GKI_API extern UINT32 GKI_get_os_tick_count(void);

/* Exception handling
*/
GKI_API extern void    GKI_exception (UINT16, char *);

#if GKI_DEBUG == TRUE
GKI_API extern void    GKI_PrintBufferUsage(UINT8 *p_num_pools, UINT16 *p_cur_used);
GKI_API extern void    GKI_PrintBuffer(void);
GKI_API extern void    GKI_print_task(void);
#else
#undef GKI_PrintBufferUsage
#define GKI_PrintBuffer() NULL
#endif

#ifdef __cplusplus
}
#endif


#endif
