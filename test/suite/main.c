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

#include "base.h"
#include "cases/cases.h"
#include "support/callbacks.h"
#include "support/hal.h"
#include "support/pan.h"

#define GRAY  "\x1b[0;37m"
#define GREEN "\x1b[0;32m"
#define RED   "\x1b[0;31m"

const bt_interface_t *bt_interface;
bt_bdaddr_t bt_remote_bdaddr;

static bool parse_bdaddr(const char *str, bt_bdaddr_t *addr) {
  if (!addr) {
    return false;
  }

  int v[6];
  if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
    return false;
  }

  for (int i = 0; i < 6; ++i) {
    addr->address[i] = (uint8_t)v[i];
  }

  return true;
}

int main(int argc, char **argv) {
  if (argc < 2 || !parse_bdaddr(argv[1], &bt_remote_bdaddr)) {
    printf("Usage: %s <bdaddr>\n", argv[0]);
    return -1;
  }

  if (!hal_open(callbacks_get_adapter_struct())) {
    printf("Unable to open Bluetooth HAL.\n");
    return 1;
  }

  if (!pan_init()) {
    printf("Unable to initialize PAN.\n");
    return 2;
  }

  int pass = 0;
  int fail = 0;
  int case_num = 0;

  // Run through the sanity suite.
  for (size_t i = 0; i < sanity_suite_size; ++i) {
    callbacks_init();
    if (sanity_suite[i].function()) {
      printf("[%4d] %-64s [%sPASS%s]\n", ++case_num, sanity_suite[i].function_name, GREEN, GRAY);
      ++pass;
    } else {
      printf("[%4d] %-64s [%sFAIL%s]\n", ++case_num, sanity_suite[i].function_name, RED, GRAY);
      ++fail;
    }
    callbacks_cleanup();
  }

  // If there was a failure in the sanity suite, don't bother running the rest of the tests.
  if (fail) {
    printf("\n%sSanity suite failed with %d errors.%s\n", RED, fail, GRAY);
    hal_close();
    return 0;
  }

  // Run the full test suite.
  for (size_t i = 0; i < test_suite_size; ++i) {
    callbacks_init();
    CALL_AND_WAIT(bt_interface->enable(false), adapter_state_changed);
    if (test_suite[i].function()) {
      printf("[%4d] %-64s [%sPASS%s]\n", ++case_num, test_suite[i].function_name, GREEN, GRAY);
      ++pass;
    } else {
      printf("[%4d] %-64s [%sFAIL%s]\n", ++case_num, test_suite[i].function_name, RED, GRAY);
      ++fail;
    }
    CALL_AND_WAIT(bt_interface->disable(), adapter_state_changed);
    callbacks_cleanup();
  }

  printf("\n");

  if (fail) {
    printf("%d/%d tests failed. See above for failed test cases.\n", fail, test_suite_size);
  } else {
    printf("All tests passed!\n");
  }

  hal_close();
  return 0;
}
