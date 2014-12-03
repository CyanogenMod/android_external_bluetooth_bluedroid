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

#define LOG_TAG "bt_osi_alarm"

#include <assert.h>
#include <errno.h>
#include <hardware/bluetooth.h>
#include <inttypes.h>
#include <time.h>
#include <utils/Log.h>

#include "alarm.h"
#include "list.h"
#include "osi.h"

struct alarm_t {
  // The lock is held while the callback for this alarm is being executed.
  // It allows us to release the coarse-grained monitor lock while a potentially
  // long-running callback is executing. |alarm_cancel| uses this lock to provide
  // a guarantee to its caller that the callback will not be in progress when it
  // returns.
  pthread_mutex_t callback_lock;
  period_ms_t deadline;
  alarm_callback_t callback;
  void *data;
};

extern bt_os_callouts_t *bt_os_callouts;

// If the next wakeup time is less than this threshold, we should acquire
// a wakelock instead of setting a wake alarm so we're not bouncing in
// and out of suspend frequently. This value is externally visible to allow
// unit tests to run faster. It should not be modified by production code.
int64_t TIMER_INTERVAL_FOR_WAKELOCK_IN_MS = 3000;
static const clockid_t CLOCK_ID = CLOCK_BOOTTIME;
static const char *WAKE_LOCK_ID = "bluedroid_timer";

// This mutex ensures that the |alarm_set|, |alarm_cancel|, and alarm callback
// functions execute serially and not concurrently. As a result, this mutex also
// protects the |alarms| list.
static pthread_mutex_t monitor;
static list_t *alarms;
static timer_t timer;
static bool timer_set;

static bool lazy_initialize(void);
static period_ms_t now(void);
static void timer_callback(void *data);
static void reschedule(void);

alarm_t *alarm_new(void) {
  // Make sure we have a list we can insert alarms into.
  if (!alarms && !lazy_initialize())
    return NULL;

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);

  alarm_t *ret = calloc(1, sizeof(alarm_t));
  if (!ret) {
    ALOGE("%s unable to allocate memory for alarm.", __func__);
    goto error;
  }

  // Make this a recursive mutex to make it safe to call |alarm_cancel| from
  // within the callback function of the alarm.
  int error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  if (error) {
    ALOGE("%s unable to create a recursive mutex: %s", __func__, strerror(error));
    goto error;
  }

  error = pthread_mutex_init(&ret->callback_lock, &attr);
  if (error) {
    ALOGE("%s unable to initialize mutex: %s", __func__, strerror(error));
    goto error;
  }

  pthread_mutexattr_destroy(&attr);
  return ret;

error:;
  pthread_mutexattr_destroy(&attr);
  free(ret);
  return NULL;
}

void alarm_free(alarm_t *alarm) {
  if (!alarm)
    return;

  alarm_cancel(alarm);
  pthread_mutex_destroy(&alarm->callback_lock);
  free(alarm);
}

// Runs in exclusion with alarm_cancel and timer_callback.
void alarm_set(alarm_t *alarm, period_ms_t deadline, alarm_callback_t cb, void *data) {
  assert(alarms != NULL);
  assert(alarm != NULL);
  assert(cb != NULL);

  pthread_mutex_lock(&monitor);

  // If the alarm is currently set and it's at the start of the list,
  // we'll need to re-schedule since we've adjusted the earliest deadline.
  bool needs_reschedule = (!list_is_empty(alarms) && list_front(alarms) == alarm);
  if (alarm->callback)
    list_remove(alarms, alarm);

  alarm->deadline = now() + deadline;
  alarm->callback = cb;
  alarm->data = data;

  // Add it into the timer list sorted by deadline (earliest deadline first).
  if (list_is_empty(alarms))
    list_prepend(alarms, alarm);
  else
    for (list_node_t *node = list_begin(alarms); node != list_end(alarms); node = list_next(node)) {
      list_node_t *next = list_next(node);
      if (next == list_end(alarms) || ((alarm_t *)list_node(next))->deadline >= alarm->deadline) {
        list_insert_after(alarms, node, alarm);
        break;
      }
    }

  // If the new alarm has the earliest deadline, we need to re-evaluate our schedule.
  if (needs_reschedule || (!list_is_empty(alarms) && list_front(alarms) == alarm))
    reschedule();

  pthread_mutex_unlock(&monitor);
}

void alarm_cancel(alarm_t *alarm) {
  assert(alarms != NULL);
  assert(alarm != NULL);

  pthread_mutex_lock(&monitor);

  bool needs_reschedule = (!list_is_empty(alarms) && list_front(alarms) == alarm);

  list_remove(alarms, alarm);
  alarm->deadline = 0;
  alarm->callback = NULL;
  alarm->data = NULL;

  if (needs_reschedule)
    reschedule();

  pthread_mutex_unlock(&monitor);

  // If the callback for |alarm| is in progress, wait here until it completes.
  pthread_mutex_lock(&alarm->callback_lock);
  pthread_mutex_unlock(&alarm->callback_lock);
}

static bool lazy_initialize(void) {
  assert(alarms == NULL);

  pthread_mutex_init(&monitor, NULL);

  alarms = list_new(NULL);
  if (!alarms) {
    ALOGE("%s unable to allocate alarm list.", __func__);
    return false;
  }

  return true;
}

static period_ms_t now(void) {
  assert(alarms != NULL);

  struct timespec ts;
  if (clock_gettime(CLOCK_ID, &ts) == -1) {
    ALOGE("%s unable to get current time: %s", __func__, strerror(errno));
    return 0;
  }

  return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

// Warning: this function is called in the context of an unknown thread.
// As a result, it must be thread-safe relative to other operations on
// the alarm list.
static void timer_callback(void *ptr) {
  alarm_t *alarm = (alarm_t *)ptr;
  assert(alarm != NULL);

  pthread_mutex_lock(&monitor);

  bool alarm_valid = list_remove(alarms, alarm);
  alarm_callback_t callback = alarm->callback;
  void *data = alarm->data;

  alarm->deadline = 0;
  alarm->callback = NULL;
  alarm->data = NULL;

  reschedule();

  // The alarm was cancelled before we got to it. Release the monitor
  // lock and exit right away since there's nothing left to do.
  if (!alarm_valid) {
    pthread_mutex_unlock(&monitor);
    return;
  }

  // Downgrade lock.
  pthread_mutex_lock(&alarm->callback_lock);
  pthread_mutex_unlock(&monitor);

  callback(data);

  pthread_mutex_unlock(&alarm->callback_lock);
}

// NOTE: must be called with monitor lock.
static void reschedule(void) {
  assert(alarms != NULL);

  if (timer_set) {
    timer_delete(timer);
    timer_set = false;
  }

  if (list_is_empty(alarms)) {
    bt_os_callouts->release_wake_lock(WAKE_LOCK_ID);
    return;
  }

  alarm_t *next = list_front(alarms);
  if (!next) {
    bt_os_callouts->release_wake_lock(WAKE_LOCK_ID);
    return;
  }
  int64_t next_exp = next->deadline - now();
  if (next_exp < TIMER_INTERVAL_FOR_WAKELOCK_IN_MS) {
    int status = bt_os_callouts->acquire_wake_lock(WAKE_LOCK_ID);
    if (status != BT_STATUS_SUCCESS) {
      ALOGE("%s unable to acquire wake lock: %d", __func__, status);
      return;
    }

    struct sigevent sigevent;
    memset(&sigevent, 0, sizeof(sigevent));
    sigevent.sigev_notify = SIGEV_THREAD;
    sigevent.sigev_notify_function = (void (*)(union sigval))timer_callback;
    sigevent.sigev_value.sival_ptr = next;
    if (timer_create(CLOCK_ID, &sigevent, &timer) == -1) {
      ALOGE("%s unable to create timer: %s", __func__, strerror(errno));
      return;
    }

    struct itimerspec wakeup_time;
    memset(&wakeup_time, 0, sizeof(wakeup_time));
    wakeup_time.it_value.tv_sec = (next->deadline / 1000);
    wakeup_time.it_value.tv_nsec = (next->deadline % 1000) * 1000000LL;
    if (timer_settime(timer, TIMER_ABSTIME, &wakeup_time, NULL) == -1) {
      ALOGE("%s unable to set timer: %s", __func__, strerror(errno));
      timer_delete(timer);
      return;
    }
    timer_set = true;
  } else {
    if (!bt_os_callouts->set_wake_alarm(next_exp, true, timer_callback, next))
      ALOGE("%s unable to set wake alarm for %" PRId64 "ms.", __func__, next_exp);

    bt_os_callouts->release_wake_lock(WAKE_LOCK_ID);
  }
}
