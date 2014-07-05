#include <gtest/gtest.h>

extern "C" {
#include "thread.h"
#include "osi.h"
}

TEST(ThreadTest, test_new_simple) {
  thread_t *thread = thread_new("test_thread");
  ASSERT_TRUE(thread != NULL);
  thread_free(thread);
}

TEST(ThreadTest, test_free_simple) {
  thread_t *thread = thread_new("test_thread");
  thread_free(thread);
}

TEST(ThreadTest, test_name) {
  thread_t *thread = thread_new("test_name");
  ASSERT_STREQ(thread_name(thread), "test_name");
  thread_free(thread);
}

TEST(ThreadTest, test_long_name) {
  thread_t *thread = thread_new("0123456789abcdef");
  ASSERT_STREQ("0123456789abcdef", thread_name(thread));
  thread_free(thread);
}

TEST(ThreadTest, test_very_long_name) {
  thread_t *thread = thread_new("0123456789abcdefg");
  ASSERT_STREQ("0123456789abcdef", thread_name(thread));
  thread_free(thread);
}
