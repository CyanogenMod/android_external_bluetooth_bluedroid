#pragma once

#include <stdbool.h>
#include <stdint.h>

#define UNUSED __attribute__((unused))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
