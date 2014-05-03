#define LOG_TAG "osi_semaphore"

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/eventfd.h>
#include <utils/Log.h>

#include "semaphore.h"

#if !defined(EFD_SEMAPHORE)
#  define EFD_SEMAPHORE (1 << 0)
#endif

struct semaphore_t {
  int fd;
};

semaphore_t *semaphore_new(unsigned int value) {
  semaphore_t *ret = malloc(sizeof(semaphore_t));
  if (ret) {
    ret->fd = eventfd(value, EFD_SEMAPHORE);
    if (ret->fd == -1) {
      ALOGE("%s unable to allocate semaphore: %s", __func__, strerror(errno));
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

void semaphore_free(semaphore_t *semaphore) {
  if (semaphore->fd != -1)
    close(semaphore->fd);
  free(semaphore);
}

void semaphore_wait(semaphore_t *semaphore) {
  assert(semaphore != NULL);
  assert(semaphore->fd != -1);

  uint64_t value;
  if (eventfd_read(semaphore->fd, &value) == -1)
    ALOGE("%s unable to wait on semaphore: %s", __func__, strerror(errno));
}

void semaphore_post(semaphore_t *semaphore) {
  assert(semaphore != NULL);
  assert(semaphore->fd != -1);

  if (eventfd_write(semaphore->fd, 1ULL) == -1)
    ALOGE("%s unable to post to semaphore: %s", __func__, strerror(errno));
}
