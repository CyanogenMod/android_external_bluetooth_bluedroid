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
#include "support/adapter.h"
#include "support/callbacks.h"
#include "support/property.h"

static bt_state_t state;
static int property_count = 0;
static bt_property_t *properties = NULL;
static bt_discovery_state_t discovery_state;

bt_state_t adapter_get_state() {
  return state;
}

int adapter_get_property_count() {
  return property_count;
}

bt_property_t *adapter_get_property(bt_property_type_t type) {
  for (int i = 0; i < property_count; ++i) {
    if (properties[i].type == type) {
      return &properties[i];
    }
  }

  return NULL;
}

bt_discovery_state_t adapter_get_discovery_state() {
  return discovery_state;
}

// callback
void adapter_state_changed(bt_state_t new_state) {
  state = new_state;
  CALLBACK_RET();
}

// callback
void adapter_properties(bt_status_t status,
                        int num_properties,
                        bt_property_t *new_properties) {
  property_free_array(properties, property_count);
  properties = property_copy_array(new_properties, num_properties);
  property_count = num_properties;

  CALLBACK_RET();
}

// callback
void discovery_state_changed(bt_discovery_state_t state) {
  discovery_state = state;
  CALLBACK_RET();
}
