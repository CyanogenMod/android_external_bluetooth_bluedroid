#include <gtest/gtest.h>

extern "C" {
#include "thread.h"
#include "osi.h"
}

void *start_routine(void *arg)
{
  return arg;
}

TEST(ThreadTest, test_new_simple) {
  thread_t *thread = thread_create("test_thread", &start_routine, NULL);
  ASSERT_TRUE(thread != NULL);
  thread_join(thread, NULL);
}

TEST(ThreadTest, test_join_simple) {
  thread_t *thread = thread_create("test_thread", &start_routine, NULL);
  thread_join(thread, NULL);
}

TEST(ThreadTest, test_name) {
  thread_t *thread = thread_create("test_name", &start_routine, NULL);
  ASSERT_STREQ(thread_name(thread), "test_name");
  thread_join(thread, NULL);
}

TEST(ThreadTest, test_long_name) {
  thread_t *thread = thread_create("0123456789abcdef", &start_routine, NULL);
  ASSERT_STREQ("0123456789abcdef", thread_name(thread));
  thread_join(thread, NULL);
}

TEST(ThreadTest, test_very_long_name) {
  thread_t *thread = thread_create("0123456789abcdefg", &start_routine, NULL);
  ASSERT_STREQ("0123456789abcdef", thread_name(thread));
  thread_join(thread, NULL);
}

TEST(ThreadTest, test_return) {
  int arg = 10;
  void *ret;
  thread_t *thread = thread_create("test", &start_routine, &arg);
  thread_join(thread, &ret);
  ASSERT_EQ(ret, &arg);
}
