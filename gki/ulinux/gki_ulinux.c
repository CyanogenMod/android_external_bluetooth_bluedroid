/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

/****************************************************************************
**
**  Name        gki_linux_pthreads.c
**
**  Function    pthreads version of Linux GKI. This version is used for
**              settop projects that already use pthreads and not pth.
**
*****************************************************************************/
#include "bt_target.h"

#include <assert.h>
#include <sys/times.h>

#include "gki_int.h"
#include "bt_utils.h"

#define LOG_TAG "GKI_LINUX"

#include <utils/Log.h>
#include <hardware/bluetooth.h>

/*****************************************************************************
**  Constants & Macros
******************************************************************************/

#define SCHED_NORMAL 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_BATCH 3

#define NANOSEC_PER_MILLISEC    1000000
#define NSEC_PER_SEC            (1000 * NANOSEC_PER_MILLISEC)
#define USEC_PER_SEC            1000000
#define NSEC_PER_USEC           1000

#define WAKE_LOCK_ID "bluedroid_timer"

#if GKI_DYNAMIC_MEMORY == FALSE
tGKI_CB   gki_cb;
#endif

#ifndef GKI_SHUTDOWN_EVT
#define GKI_SHUTDOWN_EVT    APPL_EVT_7
#endif

/*****************************************************************************
**  Local type definitions
******************************************************************************/

typedef struct
{
    UINT8 task_id;          /* GKI task id */
    TASKPTR task_entry;     /* Task entry function*/
    UINT32 params;          /* Extra params to pass to task entry function */
} gki_pthread_info_t;

// Alarm service structure used to pass up via JNI to the bluetooth
// app in order to create a wakeable Alarm.
typedef struct
{
    UINT32 ticks_scheduled;
    UINT64 timer_started_us;
    UINT64 timer_last_expired_us;
    bool wakelock;
} alarm_service_t;

/*****************************************************************************
**  Static variables
******************************************************************************/

gki_pthread_info_t gki_pthread_info[GKI_MAX_TASKS];

// Only a single alarm is used to wake bluedroid.
// NOTE: Must be manipulated with the GKI_disable() lock held.
static alarm_service_t alarm_service;

static timer_t posix_timer;
static bool timer_created;


/*****************************************************************************
**  Externs
******************************************************************************/

extern bt_os_callouts_t *bt_os_callouts;

/*****************************************************************************
**  Functions
******************************************************************************/

static UINT64 now_us()
{
    struct timespec ts_now;
    clock_gettime(CLOCK_BOOTTIME, &ts_now);
    return ((UINT64)ts_now.tv_sec * USEC_PER_SEC) + ((UINT64)ts_now.tv_nsec / NSEC_PER_USEC);
}

static bool set_nonwake_alarm(UINT64 delay_millis)
{
    if (!timer_created)
    {
        ALOGE("%s timer is not available, not setting timer for %llums", __func__, delay_millis);
        return false;
    }

    const UINT64 now = now_us();
    alarm_service.timer_started_us = now;

    UINT64 prev_timer_delay = 0;
    if (alarm_service.timer_last_expired_us)
        prev_timer_delay = now - alarm_service.timer_last_expired_us;

    UINT64 delay_micros = delay_millis * 1000;
    if (delay_micros > prev_timer_delay)
        delay_micros -= prev_timer_delay;
    else
        delay_micros = 1;

    struct itimerspec new_value;
    memset(&new_value, 0, sizeof(new_value));
    new_value.it_value.tv_sec = (delay_micros / USEC_PER_SEC);
    new_value.it_value.tv_nsec = (delay_micros % USEC_PER_SEC) * NSEC_PER_USEC;
    if (timer_settime(posix_timer, 0, &new_value, NULL) == -1)
    {
        ALOGE("%s unable to set timer: %s", __func__, strerror(errno));
        return false;
    }
    return true;
}

/** Callback from Java thread after alarm from AlarmService fires. */
static void bt_alarm_cb(void *data)
{
    UINT32 ticks_taken = 0;

    alarm_service.timer_last_expired_us = now_us();
    if (alarm_service.timer_last_expired_us > alarm_service.timer_started_us)
    {
        ticks_taken = GKI_MS_TO_TICKS((alarm_service.timer_last_expired_us
                                       - alarm_service.timer_started_us) / 1000);
    } else {
        // this could happen on some platform
        ALOGE("%s now_us %lld less than %lld", __func__, alarm_service.timer_last_expired_us,
              alarm_service.timer_started_us);
    }

    GKI_timer_update(ticks_taken > alarm_service.ticks_scheduled
                   ? ticks_taken : alarm_service.ticks_scheduled);
}

/** NOTE: This is only called on init and may be called without the GKI_disable()
  * lock held.
  */
static void alarm_service_init()
{
    alarm_service.ticks_scheduled = 0;
    alarm_service.timer_started_us = 0;
    alarm_service.timer_last_expired_us = 0;
    alarm_service.wakelock = FALSE;
    raise_priority_a2dp(TASK_JAVA_ALARM);
}

/** Requests an alarm from AlarmService to fire when the next
  * timer in the timer queue is set to expire. Only takes a wakelock
  * if the timer tick expiration is a short interval in the future
  * and releases the wakelock if the timer is a longer interval
  * or if there are no more timers in the queue.
  *
  * NOTE: Must be called with GKI_disable() lock held.
  */
void alarm_service_reschedule()
{
    int32_t ticks_till_next_exp = GKI_ready_to_sleep();

    assert(ticks_till_next_exp >= 0);
    alarm_service.ticks_scheduled = ticks_till_next_exp;

    // No more timers remaining. Release wakelock if we're holding one.
    if (ticks_till_next_exp == 0)
    {
        alarm_service.timer_last_expired_us = 0;
        alarm_service.timer_started_us = 0;
        if (alarm_service.wakelock)
        {
            ALOGV("%s releasing wake lock.", __func__);
            alarm_service.wakelock = false;
            int rc = bt_os_callouts->release_wake_lock(WAKE_LOCK_ID);
            if (rc != BT_STATUS_SUCCESS)
            {
                ALOGE("%s unable to release wake lock with no timers: %d", __func__, rc);
            }
        }
        ALOGV("%s no more alarms.", __func__);
        return;
    }

    UINT64 ticks_in_millis = GKI_TICKS_TO_MS(ticks_till_next_exp);
    if (ticks_in_millis <= GKI_TIMER_INTERVAL_FOR_WAKELOCK)
    {
        // The next deadline is close, just take a wakelock and set a regular (non-wake) timer.
        if (!alarm_service.wakelock)
        {
            int rc = bt_os_callouts->acquire_wake_lock(WAKE_LOCK_ID);
            if (rc != BT_STATUS_SUCCESS)
            {
                ALOGE("%s unable to acquire wake lock: %d", __func__, rc);
                return;
            }
            alarm_service.wakelock = true;
        }
        ALOGV("%s acquired wake lock, setting short alarm (%lldms).", __func__, ticks_in_millis);

        if (!set_nonwake_alarm(ticks_in_millis))
        {
            ALOGE("%s unable to set short alarm.", __func__);
        }
    } else {
        // The deadline is far away, set a wake alarm and release wakelock if we're holding it.
        alarm_service.timer_started_us = now_us();
        alarm_service.timer_last_expired_us = 0;
        if (!bt_os_callouts->set_wake_alarm(ticks_in_millis, true, bt_alarm_cb, &alarm_service))
        {
            ALOGE("%s unable to set long alarm, releasing wake lock anyway.", __func__);
        } else {
            ALOGV("%s set long alarm (%lldms), releasing wake lock.", __func__, ticks_in_millis);
        }

        if (alarm_service.wakelock)
        {
            alarm_service.wakelock = false;
            bt_os_callouts->release_wake_lock(WAKE_LOCK_ID);
        }
    }
}


/*****************************************************************************
**
** Function        gki_task_entry
**
** Description     GKI pthread callback
**
** Returns         void
**
*******************************************************************************/
static void gki_task_entry(UINT32 params)
{
    gki_pthread_info_t *p_pthread_info = (gki_pthread_info_t *)params;
    gki_cb.os.thread_id[p_pthread_info->task_id] = pthread_self();

    prctl(PR_SET_NAME, (unsigned long)gki_cb.com.OSTName[p_pthread_info->task_id], 0, 0, 0);

    ALOGI("gki_task_entry task_id=%i [%s] starting\n", p_pthread_info->task_id,
                gki_cb.com.OSTName[p_pthread_info->task_id]);

    /* Call the actual thread entry point */
    (p_pthread_info->task_entry)(p_pthread_info->params);

    ALOGI("gki_task task_id=%i [%s] terminating\n", p_pthread_info->task_id,
                gki_cb.com.OSTName[p_pthread_info->task_id]);

    pthread_exit(0);    /* GKI tasks have no return value */
}

/*******************************************************************************
**
** Function         GKI_init
**
** Description      This function is called once at startup to initialize
**                  all the timer structures.
**
** Returns          void
**
*******************************************************************************/

void GKI_init(void)
{
    pthread_mutexattr_t attr;
    tGKI_OS             *p_os;

    memset (&gki_cb, 0, sizeof (gki_cb));

    gki_buffer_init();
    gki_timers_init();
    alarm_service_init();

    gki_cb.com.OSTicks = (UINT32) times(0);

    pthread_mutexattr_init(&attr);

#ifndef __CYGWIN__
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    p_os = &gki_cb.os;
    pthread_mutex_init(&p_os->GKI_mutex, &attr);
    /* pthread_mutex_init(&GKI_sched_mutex, NULL); */
#if (GKI_DEBUG == TRUE)
    pthread_mutex_init(&p_os->GKI_trace_mutex, NULL);
#endif
    /* pthread_mutex_init(&thread_delay_mutex, NULL); */  /* used in GKI_delay */
    /* pthread_cond_init (&thread_delay_cond, NULL); */

    struct sigevent sigevent;
    memset(&sigevent, 0, sizeof(sigevent));
    sigevent.sigev_notify = SIGEV_THREAD;
    sigevent.sigev_notify_function = (void (*)(union sigval))bt_alarm_cb;
    sigevent.sigev_value.sival_ptr = NULL;
    if (timer_create(CLOCK_REALTIME, &sigevent, &posix_timer) == -1) {
        ALOGE("%s unable to create POSIX timer: %s", __func__, strerror(errno));
        timer_created = false;
    } else {
        timer_created = true;
    }
}


/*******************************************************************************
**
** Function         GKI_get_os_tick_count
**
** Description      This function is called to retrieve the native OS system tick.
**
** Returns          Tick count of native OS.
**
*******************************************************************************/
UINT32 GKI_get_os_tick_count(void)
{
    return gki_cb.com.OSTicks;
}

/*******************************************************************************
**
** Function         GKI_create_task
**
** Description      This function is called to create a new OSS task.
**
** Parameters:      task_entry  - (input) pointer to the entry function of the task
**                  task_id     - (input) Task id is mapped to priority
**                  taskname    - (input) name given to the task
**                  stack       - (input) pointer to the top of the stack (highest memory location)
**                  stacksize   - (input) size of the stack allocated for the task
**
** Returns          GKI_SUCCESS if all OK, GKI_FAILURE if any problem
**
** NOTE             This function take some parameters that may not be needed
**                  by your particular OS. They are here for compatability
**                  of the function prototype.
**
*******************************************************************************/
UINT8 GKI_create_task (TASKPTR task_entry, UINT8 task_id, INT8 *taskname, UINT16 *stack, UINT16 stacksize)
{
    UINT16  i;
    UINT8   *p;
    struct sched_param param;
    int policy, ret = 0;
    pthread_attr_t attr1;
    UNUSED(stack);
    UNUSED(stacksize);

    GKI_TRACE( "GKI_create_task %x %d %s %x %d", (int)task_entry, (int)task_id,
            (char*) taskname, (int) stack, (int)stacksize);

    if (task_id >= GKI_MAX_TASKS)
    {
        ALOGE("Error! task ID > max task allowed");
        return (GKI_FAILURE);
    }


    gki_cb.com.OSRdyTbl[task_id]    = TASK_READY;
    gki_cb.com.OSTName[task_id]     = taskname;
    gki_cb.com.OSWaitTmr[task_id]   = 0;
    gki_cb.com.OSWaitEvt[task_id]   = 0;

    /* Initialize mutex and condition variable objects for events and timeouts */
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);

    pthread_mutex_init(&gki_cb.os.thread_evt_mutex[task_id], NULL);
    pthread_cond_init (&gki_cb.os.thread_evt_cond[task_id], &cond_attr);
    pthread_mutex_init(&gki_cb.os.thread_timeout_mutex[task_id], NULL);
    pthread_cond_init (&gki_cb.os.thread_timeout_cond[task_id], NULL);

    pthread_attr_init(&attr1);
    /* by default, pthread creates a joinable thread */
#if ( FALSE == GKI_PTHREAD_JOINABLE )
    pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);

    GKI_TRACE("GKI creating task %i\n", task_id);
#else
    GKI_TRACE("GKI creating JOINABLE task %i\n", task_id);
#endif

    /* On Android, the new tasks starts running before 'gki_cb.os.thread_id[task_id]' is initialized */
    /* Pass task_id to new task so it can initialize gki_cb.os.thread_id[task_id] for it calls GKI_wait */
    gki_pthread_info[task_id].task_id = task_id;
    gki_pthread_info[task_id].task_entry = task_entry;
    gki_pthread_info[task_id].params = 0;

    ret = pthread_create( &gki_cb.os.thread_id[task_id],
              &attr1,
              (void *)gki_task_entry,
              &gki_pthread_info[task_id]);

    if (ret != 0)
    {
         ALOGE("pthread_create failed(%d), %s!", ret, taskname);
         return GKI_FAILURE;
    }

    if(pthread_getschedparam(gki_cb.os.thread_id[task_id], &policy, &param)==0)
     {
#if (GKI_LINUX_BASE_POLICY!=GKI_SCHED_NORMAL)
#if defined(PBS_SQL_TASK)
         if (task_id == PBS_SQL_TASK)
         {
             GKI_TRACE("PBS SQL lowest priority task");
             policy = SCHED_NORMAL;
         }
         else
#endif
#endif
         {
             /* check if define in gki_int.h is correct for this compile environment! */
             policy = GKI_LINUX_BASE_POLICY;
#if (GKI_LINUX_BASE_POLICY != GKI_SCHED_NORMAL)
             param.sched_priority = GKI_LINUX_BASE_PRIORITY - task_id - 2;
#else
             param.sched_priority = 0;
#endif
         }
         pthread_setschedparam(gki_cb.os.thread_id[task_id], policy, &param);
     }

    GKI_TRACE( "Leaving GKI_create_task %x %d %x %s %x %d\n",
              (int)task_entry,
              (int)task_id,
              (int)gki_cb.os.thread_id[task_id],
              (char*)taskname,
              (int)stack,
              (int)stacksize);

    return (GKI_SUCCESS);
}

void GKI_destroy_task(UINT8 task_id)
{
#if ( FALSE == GKI_PTHREAD_JOINABLE )
        int i = 0;
#else
        int result;
#endif
    if (gki_cb.com.OSRdyTbl[task_id] != TASK_DEAD)
    {
        gki_cb.com.OSRdyTbl[task_id] = TASK_DEAD;

        /* paranoi settings, make sure that we do not execute any mailbox events */
        gki_cb.com.OSWaitEvt[task_id] &= ~(TASK_MBOX_0_EVT_MASK|TASK_MBOX_1_EVT_MASK|
                                            TASK_MBOX_2_EVT_MASK|TASK_MBOX_3_EVT_MASK);

#if (GKI_NUM_TIMERS > 0)
        gki_cb.com.OSTaskTmr0R[task_id] = 0;
        gki_cb.com.OSTaskTmr0 [task_id] = 0;
#endif

#if (GKI_NUM_TIMERS > 1)
        gki_cb.com.OSTaskTmr1R[task_id] = 0;
        gki_cb.com.OSTaskTmr1 [task_id] = 0;
#endif

#if (GKI_NUM_TIMERS > 2)
        gki_cb.com.OSTaskTmr2R[task_id] = 0;
        gki_cb.com.OSTaskTmr2 [task_id] = 0;
#endif

#if (GKI_NUM_TIMERS > 3)
        gki_cb.com.OSTaskTmr3R[task_id] = 0;
        gki_cb.com.OSTaskTmr3 [task_id] = 0;
#endif

        GKI_send_event(task_id, EVENT_MASK(GKI_SHUTDOWN_EVT));

#if ( FALSE == GKI_PTHREAD_JOINABLE )
        i = 0;

        while ((gki_cb.com.OSWaitEvt[task_id] != 0) && (++i < 10))
            usleep(100 * 1000);
#else
        result = pthread_join( gki_cb.os.thread_id[task_id], NULL );
        if ( result < 0 )
        {
            ALOGE( "pthread_join() FAILED: result: %d", result );
        }
#endif
        GKI_exit_task(task_id);
        ALOGI( "GKI_shutdown(): task [%s] terminated\n", gki_cb.com.OSTName[task_id]);
    }
}


/*******************************************************************************
**
** Function         GKI_task_self_cleanup
**
** Description      This function is used in the case when the calling thread
**                  is exiting itself. The GKI_destroy_task function can not be
**                  used in this case due to the pthread_join call. The function
**                  cleans up GKI control block associated to the terminating
**                  thread.
**
** Parameters:      task_id     - (input) Task id is used for sanity check to
**                                 make sure the calling thread is in the right
**                                 context.
**
** Returns          None
**
*******************************************************************************/
void GKI_task_self_cleanup(UINT8 task_id)
{
    UINT8 my_task_id = GKI_get_taskid();

    if (task_id != my_task_id)
    {
        ALOGE("%s: Wrong context - current task %d is not the given task id %d",\
                      __FUNCTION__, my_task_id, task_id);
        return;
    }

    if (gki_cb.com.OSRdyTbl[task_id] != TASK_DEAD)
    {
        /* paranoi settings, make sure that we do not execute any mailbox events */
        gki_cb.com.OSWaitEvt[task_id] &= ~(TASK_MBOX_0_EVT_MASK|TASK_MBOX_1_EVT_MASK|
                                            TASK_MBOX_2_EVT_MASK|TASK_MBOX_3_EVT_MASK);

#if (GKI_NUM_TIMERS > 0)
        gki_cb.com.OSTaskTmr0R[task_id] = 0;
        gki_cb.com.OSTaskTmr0 [task_id] = 0;
#endif

#if (GKI_NUM_TIMERS > 1)
        gki_cb.com.OSTaskTmr1R[task_id] = 0;
        gki_cb.com.OSTaskTmr1 [task_id] = 0;
#endif

#if (GKI_NUM_TIMERS > 2)
        gki_cb.com.OSTaskTmr2R[task_id] = 0;
        gki_cb.com.OSTaskTmr2 [task_id] = 0;
#endif

#if (GKI_NUM_TIMERS > 3)
        gki_cb.com.OSTaskTmr3R[task_id] = 0;
        gki_cb.com.OSTaskTmr3 [task_id] = 0;
#endif

        GKI_exit_task(task_id);

        /* Calling pthread_detach here to mark the thread as detached.
           Once the thread terminates, the system can reclaim its resources
           without waiting for another thread to join with.
        */
        pthread_detach(gki_cb.os.thread_id[task_id]);
    }
}

/*******************************************************************************
**
** Function         GKI_shutdown
**
** Description      shutdowns the GKI tasks/threads in from max task id to 0 and frees
**                  pthread resources!
**                  IMPORTANT: in case of join method, GKI_shutdown must be called outside
**                  a GKI thread context!
**
** Returns          void
**
*******************************************************************************/

void GKI_shutdown(void)
{
    UINT8 task_id;
#if ( FALSE == GKI_PTHREAD_JOINABLE )
    int i = 0;
#else
    int result;
#endif

#ifdef GKI_USE_DEFERED_ALLOC_BUF_POOLS
    gki_dealloc_free_queue();
#endif

    /* release threads and set as TASK_DEAD. going from low to high priority fixes
     * GKI_exception problem due to btu->hci sleep request events  */
    for (task_id = GKI_MAX_TASKS; task_id > 0; task_id--)
    {
        if (gki_cb.com.OSRdyTbl[task_id - 1] != TASK_DEAD)
        {
            gki_cb.com.OSRdyTbl[task_id - 1] = TASK_DEAD;

            /* paranoi settings, make sure that we do not execute any mailbox events */
            gki_cb.com.OSWaitEvt[task_id-1] &= ~(TASK_MBOX_0_EVT_MASK|TASK_MBOX_1_EVT_MASK|
                                                TASK_MBOX_2_EVT_MASK|TASK_MBOX_3_EVT_MASK);
            GKI_send_event(task_id - 1, EVENT_MASK(GKI_SHUTDOWN_EVT));

#if ( FALSE == GKI_PTHREAD_JOINABLE )
            i = 0;

            while ((gki_cb.com.OSWaitEvt[task_id - 1] != 0) && (++i < 10))
                usleep(100 * 1000);
#else
            result = pthread_join( gki_cb.os.thread_id[task_id-1], NULL );

            if ( result < 0 )
            {
                ALOGE( "pthread_join() FAILED: result: %d", result );
            }
#endif
            GKI_exit_task(task_id - 1);
        }
    }

    /* Destroy mutex and condition variable objects */
    pthread_mutex_destroy(&gki_cb.os.GKI_mutex);

    /*    pthread_mutex_destroy(&GKI_sched_mutex); */
#if (GKI_DEBUG == TRUE)
    pthread_mutex_destroy(&gki_cb.os.GKI_trace_mutex);
#endif
    /*    pthread_mutex_destroy(&thread_delay_mutex);
     pthread_cond_destroy (&thread_delay_cond); */
#if ( FALSE == GKI_PTHREAD_JOINABLE )
    i = 0;
#endif

    if (timer_created) {
        timer_delete(posix_timer);
        timer_created = false;
    }
}

/*****************************************************************************
**
** Function        gki_set_timer_scheduling
**
** Description     helper function to set scheduling policy and priority of btdl
**
** Returns         void
**
*******************************************************************************/

static void gki_set_timer_scheduling( void )
{
    pid_t               main_pid = getpid();
    struct sched_param  param;
    int                 policy;

    policy = sched_getscheduler(main_pid);

    if ( policy != -1 )
    {
        GKI_TRACE("gki_set_timer_scheduling(()::scheduler current policy: %d", policy);

        /* ensure highest priority in the system + 2 to allow space for read threads */
        param.sched_priority = GKI_LINUX_TIMER_TICK_PRIORITY;

        if ( 0!=sched_setscheduler(main_pid, GKI_LINUX_TIMER_POLICY, &param ) )
        {
            GKI_TRACE("sched_setscheduler() failed with error: %d", errno);
        }
    }
    else
    {
        GKI_TRACE( "getscheduler failed: %d", errno);
    }
}


/*****************************************************************************
**
** Function        GKI_run
**
** Description     Main GKI loop
**
** Returns
**
*******************************************************************************/

void GKI_run(void)
{
    /* adjust btld scheduling scheme now */
    gki_set_timer_scheduling();
    GKI_TRACE( "GKI_run(): Start/Stop GKI_timer_update_registered!" );
}


/*******************************************************************************
**
** Function         GKI_stop
**
** Description      This function is called to stop
**                  the tasks and timers when the system is being stopped
**
** Returns          void
**
** NOTE             This function is NOT called by the Broadcom stack and
**                  profiles. If you want to use it in your own implementation,
**                  put specific code here.
**
*******************************************************************************/

void GKI_stop (void)
{
    UINT8 task_id;

    /*  gki_queue_timer_cback(FALSE); */
    /* TODO - add code here if needed*/

    for(task_id = 0; task_id<GKI_MAX_TASKS; task_id++)
    {
        if(gki_cb.com.OSRdyTbl[task_id] != TASK_DEAD)
        {
            GKI_exit_task(task_id);
        }
    }
}


/*******************************************************************************
**
** Function         GKI_wait
**
** Description      This function is called by tasks to wait for a specific
**                  event or set of events. The task may specify the duration
**                  that it wants to wait for, or 0 if infinite.
**
** Parameters:      flag -    (input) the event or set of events to wait for
**                  timeout - (input) the duration that the task wants to wait
**                                    for the specific events (in system ticks)
**
**
** Returns          the event mask of received events or zero if timeout
**
*******************************************************************************/
UINT16 GKI_wait (UINT16 flag, UINT32 timeout)
{
    UINT16 evt;
    UINT8 rtask;
    struct timespec abstime = { 0, 0 };

    int sec;
    int nano_sec;

    rtask = GKI_get_taskid();

    GKI_TRACE("GKI_wait %d %x %d", (int)rtask, (int)flag, (int)timeout);

    gki_cb.com.OSWaitForEvt[rtask] = flag;

    /* protect OSWaitEvt[rtask] from modification from an other thread */
    pthread_mutex_lock(&gki_cb.os.thread_evt_mutex[rtask]);

    if (!(gki_cb.com.OSWaitEvt[rtask] & flag))
    {
        if (timeout)
        {
            clock_gettime(CLOCK_MONOTONIC, &abstime);

            /* add timeout */
            sec = timeout / 1000;
            nano_sec = (timeout % 1000) * NANOSEC_PER_MILLISEC;
            abstime.tv_nsec += nano_sec;
            if (abstime.tv_nsec > NSEC_PER_SEC)
            {
                abstime.tv_sec += (abstime.tv_nsec / NSEC_PER_SEC);
                abstime.tv_nsec = abstime.tv_nsec % NSEC_PER_SEC;
            }
            abstime.tv_sec += sec;

            pthread_cond_timedwait(&gki_cb.os.thread_evt_cond[rtask],
                    &gki_cb.os.thread_evt_mutex[rtask], &abstime);
        }
        else
        {
            pthread_cond_wait(&gki_cb.os.thread_evt_cond[rtask], &gki_cb.os.thread_evt_mutex[rtask]);
        }

        /* TODO: check, this is probably neither not needed depending on phtread_cond_wait() implmentation,
         e.g. it looks like it is implemented as a counter in which case multiple cond_signal
         should NOT be lost! */

        /* we are waking up after waiting for some events, so refresh variables
           no need to call GKI_disable() here as we know that we will have some events as we've been waking
           up after condition pending or timeout */

        if (gki_cb.com.OSTaskQFirst[rtask][0])
            gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_0_EVT_MASK;
        if (gki_cb.com.OSTaskQFirst[rtask][1])
            gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_1_EVT_MASK;
        if (gki_cb.com.OSTaskQFirst[rtask][2])
            gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_2_EVT_MASK;
        if (gki_cb.com.OSTaskQFirst[rtask][3])
            gki_cb.com.OSWaitEvt[rtask] |= TASK_MBOX_3_EVT_MASK;

        if (gki_cb.com.OSRdyTbl[rtask] == TASK_DEAD)
        {
            gki_cb.com.OSWaitEvt[rtask] = 0;
            /* unlock thread_evt_mutex as pthread_cond_wait() does auto lock when cond is met */
            pthread_mutex_unlock(&gki_cb.os.thread_evt_mutex[rtask]);
            return (EVENT_MASK(GKI_SHUTDOWN_EVT));
        }
    }

    /* Clear the wait for event mask */
    gki_cb.com.OSWaitForEvt[rtask] = 0;

    /* Return only those bits which user wants... */
    evt = gki_cb.com.OSWaitEvt[rtask] & flag;

    /* Clear only those bits which user wants... */
    gki_cb.com.OSWaitEvt[rtask] &= ~flag;

    /* unlock thread_evt_mutex as pthread_cond_wait() does auto lock mutex when cond is met */
    pthread_mutex_unlock(&gki_cb.os.thread_evt_mutex[rtask]);

    GKI_TRACE("GKI_wait %d %x %d %x done", (int)rtask, (int)flag, (int)timeout, (int)evt);
    return (evt);
}


/*******************************************************************************
**
** Function         GKI_delay
**
** Description      This function is called by tasks to sleep unconditionally
**                  for a specified amount of time. The duration is in milliseconds
**
** Parameters:      timeout -    (input) the duration in milliseconds
**
** Returns          void
**
*******************************************************************************/

void GKI_delay (UINT32 timeout)
{
    UINT8 rtask = GKI_get_taskid();
    struct timespec delay;
    int err;

    GKI_TRACE("GKI_delay %d %d", (int)rtask, (int)timeout);

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout%1000);

    /* [u]sleep can't be used because it uses SIGALRM */

    do {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno ==EINTR);

    /* Check if task was killed while sleeping */

     /* NOTE : if you do not implement task killing, you do not need this check */

    if (rtask && gki_cb.com.OSRdyTbl[rtask] == TASK_DEAD)
    {
    }

    GKI_TRACE("GKI_delay %d %d done", (int)rtask, (int)timeout);

    return;
}


/*******************************************************************************
**
** Function         GKI_send_event
**
** Description      This function is called by tasks to send events to other
**                  tasks. Tasks can also send events to themselves.
**
** Parameters:      task_id -  (input) The id of the task to which the event has to
**                  be sent
**                  event   -  (input) The event that has to be sent
**
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
*******************************************************************************/

UINT8 GKI_send_event (UINT8 task_id, UINT16 event)
{
    GKI_TRACE("GKI_send_event %d %x", task_id, event);

    if (task_id < GKI_MAX_TASKS)
    {
        /* protect OSWaitEvt[task_id] from manipulation in GKI_wait() */
        pthread_mutex_lock(&gki_cb.os.thread_evt_mutex[task_id]);

        /* Set the event bit */
        gki_cb.com.OSWaitEvt[task_id] |= event;

        pthread_cond_signal(&gki_cb.os.thread_evt_cond[task_id]);

        pthread_mutex_unlock(&gki_cb.os.thread_evt_mutex[task_id]);

        GKI_TRACE("GKI_send_event %d %x done", task_id, event);
        return ( GKI_SUCCESS );
    }
    GKI_TRACE("############## GKI_send_event FAILED!! ##################");
    return (GKI_FAILURE);
}


/*******************************************************************************
**
** Function         GKI_get_taskid
**
** Description      This function gets the currently running task ID.
**
** Returns          task ID
**
** NOTE             The Broadcom upper stack and profiles may run as a single task.
**                  If you only have one GKI task, then you can hard-code this
**                  function to return a '1'. Otherwise, you should have some
**                  OS-specific method to determine the current task.
**
*******************************************************************************/
UINT8 GKI_get_taskid (void)
{
    int i;

    pthread_t thread_id = pthread_self( );

    GKI_TRACE("GKI_get_taskid %x", (int)thread_id);

    for (i = 0; i < GKI_MAX_TASKS; i++) {
        if (gki_cb.os.thread_id[i] == thread_id) {
            //GKI_TRACE("GKI_get_taskid %x %d done", thread_id, i);
            return(i);
        }
    }

    GKI_TRACE("GKI_get_taskid: task id = -1");

    return(-1);
}


/*******************************************************************************
**
** Function         GKI_map_taskname
**
** Description      This function gets the task name of the taskid passed as arg.
**                  If GKI_MAX_TASKS is passed as arg the currently running task
**                  name is returned
**
** Parameters:      task_id -  (input) The id of the task whose name is being
**                  sought. GKI_MAX_TASKS is passed to get the name of the
**                  currently running task.
**
** Returns          pointer to task name
**
** NOTE             this function needs no customization
**
*******************************************************************************/

INT8 *GKI_map_taskname (UINT8 task_id)
{
    GKI_TRACE("GKI_map_taskname %d", task_id);

    if (task_id < GKI_MAX_TASKS)
    {
        GKI_TRACE("GKI_map_taskname %d %s done", task_id, gki_cb.com.OSTName[task_id]);
         return (gki_cb.com.OSTName[task_id]);
    }
    else if (task_id == GKI_MAX_TASKS )
    {
        return (gki_cb.com.OSTName[GKI_get_taskid()]);
    }
    else
    {
        return (INT8*)"BAD";
    }
}


/*******************************************************************************
**
** Function         GKI_enable
**
** Description      This function enables interrupts.
**
** Returns          void
**
*******************************************************************************/
void GKI_enable (void)
{
    pthread_mutex_unlock(&gki_cb.os.GKI_mutex);
}


/*******************************************************************************
**
** Function         GKI_disable
**
** Description      This function disables interrupts.
**
** Returns          void
**
*******************************************************************************/

void GKI_disable (void)
{
    pthread_mutex_lock(&gki_cb.os.GKI_mutex);
}


/*******************************************************************************
**
** Function         GKI_exception
**
** Description      This function throws an exception.
**                  This is normally only called for a nonrecoverable error.
**
** Parameters:      code    -  (input) The code for the error
**                  msg     -  (input) The message that has to be logged
**
** Returns          void
**
*******************************************************************************/

void GKI_exception (UINT16 code, char *msg)
{
    UINT8 task_id;
    int i = 0;
    FREE_QUEUE_T  *Q;
    tGKI_COM_CB *p_cb = &gki_cb.com;

    ALOGE( "GKI_exception(): Task State Table");

    for(task_id = 0; task_id < GKI_MAX_TASKS; task_id++)
    {
        ALOGE( "TASK ID [%d] task name [%s] state [%d]",
                         task_id,
                         gki_cb.com.OSTName[task_id],
                         gki_cb.com.OSRdyTbl[task_id]);
    }

    ALOGE("GKI_exception %d %s", code, msg);
    ALOGE( "********************************************************************");
    ALOGE( "* GKI_exception(): %d %s", code, msg);
    ALOGE( "********************************************************************");

#if 0//(GKI_DEBUG == TRUE)
    GKI_disable();

    if (gki_cb.com.ExceptionCnt < GKI_MAX_EXCEPTION)
    {
        EXCEPTION_T *pExp;

        pExp =  &gki_cb.com.Exception[gki_cb.com.ExceptionCnt++];
        pExp->type = code;
        pExp->taskid = GKI_get_taskid();
        strncpy((char *)pExp->msg, msg, GKI_MAX_EXCEPTION_MSGLEN - 1);
    }

    GKI_enable();
#endif
    if (code == GKI_ERROR_OUT_OF_BUFFERS)
    {
        for(i=0; i<p_cb->curr_total_no_of_pools; i++)
        {
            Q = &p_cb->freeq[p_cb->pool_list[i]];
            if (Q !=NULL)
                ALOGE("GKI_exception Buffer current cnt:%x, Total:%x", Q->cur_cnt, Q->total);
        }
    }

    GKI_TRACE("GKI_exception %d %s done", code, msg);
    return;
}

/*******************************************************************************
**
** Function         GKI_os_malloc
**
** Description      This function allocates memory
**
** Parameters:      size -  (input) The size of the memory that has to be
**                  allocated
**
** Returns          the address of the memory allocated, or NULL if failed
**
** NOTE             This function is called by the Broadcom stack when
**                  dynamic memory allocation is used. (see dyn_mem.h)
**
*******************************************************************************/
void *GKI_os_malloc (UINT32 size)
{
    return malloc(size);
}

/*******************************************************************************
**
** Function         GKI_os_free
**
** Description      This function frees memory
**
** Parameters:      size -  (input) The address of the memory that has to be
**                  freed
**
** Returns          void
**
** NOTE             This function is NOT called by the Broadcom stack and
**                  profiles. It is only called from within GKI if dynamic
**
*******************************************************************************/
void GKI_os_free (void *p_mem)
{
    free(p_mem);
}


/*******************************************************************************
**
** Function         GKI_exit_task
**
** Description      This function is called to stop a GKI task.
**
** Parameters:      task_id  - (input) the id of the task that has to be stopped
**
** Returns          void
**
** NOTE             This function is NOT called by the Broadcom stack and
**                  profiles. If you want to use it in your own implementation,
**                  put specific code here to kill a task.
**
*******************************************************************************/
void GKI_exit_task (UINT8 task_id)
{
    GKI_disable();
    gki_cb.com.OSRdyTbl[task_id] = TASK_DEAD;

    /* Destroy mutex and condition variable objects */
    pthread_mutex_destroy(&gki_cb.os.thread_evt_mutex[task_id]);
    pthread_cond_destroy (&gki_cb.os.thread_evt_cond[task_id]);
    pthread_mutex_destroy(&gki_cb.os.thread_timeout_mutex[task_id]);
    pthread_cond_destroy (&gki_cb.os.thread_timeout_cond[task_id]);

    GKI_enable();

    ALOGI("GKI_exit_task %d done", task_id);
    return;
}
