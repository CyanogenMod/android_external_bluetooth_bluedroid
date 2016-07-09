#include <gtest/gtest.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

extern "C" {
#include "reactor.h"
}

static pthread_t thread;
static volatile bool thread_running;

static void *reactor_thread(void *ptr) {
  reactor_t *reactor = (reactor_t *)ptr;

  thread_running = true;
  reactor_start(reactor);
  thread_running = false;

  return NULL;
}

static void spawn_reactor_thread(reactor_t *reactor) {
  int ret = pthread_create(&thread, NULL, reactor_thread, reactor);
  EXPECT_EQ(ret, 0);
}

static void join_reactor_thread() {
  pthread_join(thread, NULL);
}

static uint64_t get_timestamp(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

TEST(ReactorTest, reactor_new) {
  reactor_t *reactor = reactor_new();
  EXPECT_TRUE(reactor != NULL);
  reactor_free(reactor);
}

TEST(ReactorTest, reactor_free_null) {
  reactor_free(NULL);
}

TEST(ReactorTest, reactor_stop_start) {
  reactor_t *reactor = reactor_new();
  reactor_stop(reactor);
  reactor_start(reactor);
  reactor_free(reactor);
}

TEST(ReactorTest, reactor_repeated_stop_start) {
  reactor_t *reactor = reactor_new();
  for (int i = 0; i < 10; ++i) {
    reactor_stop(reactor);
    reactor_start(reactor);
  }
  reactor_free(reactor);
}

TEST(ReactorTest, reactor_multi_stop_start) {
  reactor_t *reactor = reactor_new();

  reactor_stop(reactor);
  reactor_stop(reactor);
  reactor_stop(reactor);

  reactor_start(reactor);
  reactor_start(reactor);
  reactor_start(reactor);

  reactor_free(reactor);
}

TEST(ReactorTest, reactor_start_wait_stop) {
  reactor_t *reactor = reactor_new();

  spawn_reactor_thread(reactor);
  TEMP_FAILURE_RETRY(usleep(50 * 1000));
  EXPECT_TRUE(thread_running);

  reactor_stop(reactor);
  join_reactor_thread();
  EXPECT_FALSE(thread_running);

  reactor_free(reactor);
}

TEST(ReactorTest, reactor_run_once_timeout) {
  reactor_t *reactor = reactor_new();

  uint64_t start = get_timestamp();
  reactor_status_t status = reactor_run_once_timeout(reactor, 50);
  EXPECT_GE(get_timestamp() - start, static_cast<uint64_t>(50));
  EXPECT_EQ(status, REACTOR_STATUS_TIMEOUT);

  reactor_free(reactor);
}
