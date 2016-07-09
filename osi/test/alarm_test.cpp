#include <gtest/gtest.h>
#include <hardware/bluetooth.h>
#include <unistd.h>

extern "C" {
#include "alarm.h"
#include "osi.h"
#include "semaphore.h"
}

extern int64_t TIMER_INTERVAL_FOR_WAKELOCK_IN_MS;

static semaphore_t *semaphore;
static int cb_counter;
static int lock_count;
static timer_t timer;
static alarm_cb saved_callback;
static void *saved_data;

static const uint64_t EPSILON_MS = 5;

static void msleep(uint64_t ms) {
  TEMP_FAILURE_RETRY(usleep(ms * 1000));
}

static void timer_callback(void *) {
  saved_callback(saved_data);
}

class AlarmTest : public ::testing::Test {
  protected:
    virtual void SetUp() {
      TIMER_INTERVAL_FOR_WAKELOCK_IN_MS = 100;
      cb_counter = 0;
      lock_count = 0;

      semaphore = semaphore_new(0);

      struct sigevent sigevent;
      memset(&sigevent, 0, sizeof(sigevent));
      sigevent.sigev_notify = SIGEV_THREAD;
      sigevent.sigev_notify_function = (void (*)(union sigval))timer_callback;
      sigevent.sigev_value.sival_ptr = NULL;
      timer_create(CLOCK_BOOTTIME, &sigevent, &timer);
    }

    virtual void TearDown() {
      timer_delete(timer);
    }
};

static void cb(UNUSED_ATTR void *data) {
  ++cb_counter;
  semaphore_post(semaphore);
}

static bool set_wake_alarm(uint64_t delay_millis, bool, alarm_cb cb, void *data) {
  saved_callback = cb;
  saved_data = data;

  struct itimerspec wakeup_time;
  memset(&wakeup_time, 0, sizeof(wakeup_time));
  wakeup_time.it_value.tv_sec = (delay_millis / 1000);
  wakeup_time.it_value.tv_nsec = (delay_millis % 1000) * 1000000LL;
  timer_settime(timer, 0, &wakeup_time, NULL);
  return true;
}

static int acquire_wake_lock(const char *) {
  if (!lock_count)
    lock_count = 1;
  return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char *) {
  if (lock_count)
    lock_count = 0;
  return BT_STATUS_SUCCESS;
}

static bt_os_callouts_t stub = {
  sizeof(bt_os_callouts_t),
  set_wake_alarm,
  acquire_wake_lock,
  release_wake_lock,
};

bt_os_callouts_t *bt_os_callouts = &stub;

TEST_F(AlarmTest, test_new_simple) {
  alarm_t *alarm = alarm_new();
  ASSERT_TRUE(alarm != NULL);
}

TEST_F(AlarmTest, test_free_simple) {
  alarm_t *alarm = alarm_new();
  alarm_free(alarm);
}

TEST_F(AlarmTest, test_free_null) {
  alarm_free(NULL);
}

TEST_F(AlarmTest, test_simple_cancel) {
  alarm_t *alarm = alarm_new();
  alarm_cancel(alarm);
  alarm_free(alarm);
}

TEST_F(AlarmTest, test_cancel) {
  alarm_t *alarm = alarm_new();
  alarm_set(alarm, 10, cb, NULL);
  alarm_cancel(alarm);

  msleep(10 + EPSILON_MS);

  EXPECT_EQ(cb_counter, 0);
  EXPECT_EQ(lock_count, 0);
  alarm_free(alarm);
}

TEST_F(AlarmTest, test_cancel_idempotent) {
  alarm_t *alarm = alarm_new();
  alarm_set(alarm, 10, cb, NULL);
  alarm_cancel(alarm);
  alarm_cancel(alarm);
  alarm_cancel(alarm);
  alarm_free(alarm);
}

TEST_F(AlarmTest, test_set_short) {
  alarm_t *alarm = alarm_new();
  alarm_set(alarm, 10, cb, NULL);

  EXPECT_EQ(cb_counter, 0);
  EXPECT_EQ(lock_count, 1);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 1);
  EXPECT_EQ(lock_count, 0);

  alarm_free(alarm);
}

TEST_F(AlarmTest, test_set_long) {
  alarm_t *alarm = alarm_new();
  alarm_set(alarm, TIMER_INTERVAL_FOR_WAKELOCK_IN_MS, cb, NULL);

  EXPECT_EQ(cb_counter, 0);
  EXPECT_EQ(lock_count, 0);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 1);
  EXPECT_EQ(lock_count, 0);

  alarm_free(alarm);
}

TEST_F(AlarmTest, test_set_short_short) {
  alarm_t *alarm[2] = {
    alarm_new(),
    alarm_new()
  };

  alarm_set(alarm[0], 10, cb, NULL);
  alarm_set(alarm[1], 20, cb, NULL);

  EXPECT_EQ(cb_counter, 0);
  EXPECT_EQ(lock_count, 1);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 1);
  EXPECT_EQ(lock_count, 1);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 2);
  EXPECT_EQ(lock_count, 0);

  alarm_free(alarm[0]);
  alarm_free(alarm[1]);
}

TEST_F(AlarmTest, test_set_short_long) {
  alarm_t *alarm[2] = {
    alarm_new(),
    alarm_new()
  };

  alarm_set(alarm[0], 10, cb, NULL);
  alarm_set(alarm[1], 10 + TIMER_INTERVAL_FOR_WAKELOCK_IN_MS + EPSILON_MS, cb, NULL);

  EXPECT_EQ(cb_counter, 0);
  EXPECT_EQ(lock_count, 1);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 1);
  EXPECT_EQ(lock_count, 0);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 2);
  EXPECT_EQ(lock_count, 0);

  alarm_free(alarm[0]);
  alarm_free(alarm[1]);
}

TEST_F(AlarmTest, test_set_long_long) {
  alarm_t *alarm[2] = {
    alarm_new(),
    alarm_new()
  };

  alarm_set(alarm[0], TIMER_INTERVAL_FOR_WAKELOCK_IN_MS, cb, NULL);
  alarm_set(alarm[1], 2 * TIMER_INTERVAL_FOR_WAKELOCK_IN_MS + EPSILON_MS, cb, NULL);

  EXPECT_EQ(cb_counter, 0);
  EXPECT_EQ(lock_count, 0);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 1);
  EXPECT_EQ(lock_count, 0);

  semaphore_wait(semaphore);

  EXPECT_EQ(cb_counter, 2);
  EXPECT_EQ(lock_count, 0);

  alarm_free(alarm[0]);
  alarm_free(alarm[1]);
}

// Try to catch any race conditions between the timer callback and |alarm_free|.
TEST_F(AlarmTest, test_callback_free_race) {
  for (int i = 0; i < 1000; ++i) {
    alarm_t *alarm = alarm_new();
    alarm_set(alarm, 0, cb, NULL);
    alarm_free(alarm);
  }
}
