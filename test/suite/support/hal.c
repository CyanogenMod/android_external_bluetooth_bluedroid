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
#include "support/hal.h"

static const bluetooth_device_t *bt_device;

bool hal_open(bt_callbacks_t *callbacks) {
  hw_module_t *module;
  if (hw_get_module(BT_STACK_MODULE_ID, (hw_module_t const **)&module)) {
    return false;
  }

  hw_device_t *device;
  if (module->methods->open(module, BT_STACK_MODULE_ID, &device)) {
    return false;
  }

  bt_device = (bluetooth_device_t *)device;
  bt_interface = bt_device->get_bluetooth_interface();
  if (!bt_interface) {
    bt_device->common.close((hw_device_t *)&bt_device->common);
    bt_device = NULL;
    return false;
  }

  return bt_interface->init(callbacks) == BT_STATUS_SUCCESS;
}

void hal_close() {
  if (bt_interface) {
    bt_interface->cleanup();
    bt_interface = NULL;
  }

  if (bt_device) {
    bt_device->common.close((hw_device_t *)&bt_device->common);
    bt_device = NULL;
  }
}
