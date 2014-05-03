#pragma once

struct semaphore_t;
typedef struct semaphore_t semaphore_t;

semaphore_t *semaphore_new(unsigned int value);
void semaphore_free(semaphore_t *semaphore);

void semaphore_wait(semaphore_t *semaphore);
void semaphore_post(semaphore_t *semaphore);
