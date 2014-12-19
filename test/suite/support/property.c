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
#include "support/property.h"

bt_property_t *property_copy_array(const bt_property_t *properties, size_t count) {
  bt_property_t *clone = calloc(sizeof(bt_property_t), count);
  if (!clone) {
    return NULL;
  }

  memcpy(&clone[0], &properties[0], sizeof(bt_property_t) * count);
  for (size_t i = 0; i < count; ++i) {
    clone[i].val = calloc(clone[i].len, 1);
    memcpy(clone[i].val, properties[i].val, clone[i].len);
  }

  return clone;
}

bt_property_t *property_new_name(const char *name) {
  bt_bdname_t *bdname = calloc(sizeof(bt_bdname_t), 1);
  bt_property_t *property = calloc(sizeof(bt_property_t), 1);
  if (!property || !bdname)
  {
    perror("property_new_name(): Failed to allocate memory");
    return NULL;
  }
  property->type = BT_PROPERTY_BDNAME;
  property->val = bdname;
  property->len = sizeof(bt_bdname_t);

  strlcpy((char *)bdname->name, name, sizeof(bdname->name));

  return property;
}

bt_property_t *property_new_discovery_timeout(uint32_t timeout) {
  uint32_t *val = malloc(sizeof(uint32_t));
  bt_property_t *property = malloc(sizeof(bt_property_t));
  if(!val || !property)
  {
    perror("property_new_discovery(): Failed to allocate memory");
    return NULL;
  }
  property->type = BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT;
  property->val = val;
  property->len = sizeof(uint32_t);

  *val = timeout;

  return property;
}

// Warning: not thread safe.
const char *property_extract_name(const bt_property_t *property) {
  static char name[250] = { 0 };
  if (!property || property->type != BT_PROPERTY_BDNAME || !property->val) {
    return NULL;
  }

  strncpy(name, (const char *)((bt_bdname_t *)property->val)->name, property->len);
  name[sizeof(name) - 1] = '\0';

  return name;
}

bool property_equals(const bt_property_t *p1, const bt_property_t *p2) {
  // Two null properties are not the same. May need to revisit that
  // decision when we have a test case that exercises that condition.
  if (!p1 || !p2 || p1->type != p2->type) {
    return false;
  }

  // Although the Bluetooth name is a 249-byte array, the implementation
  // treats it like a variable-length array with its size specified in the
  // property's `len` field. We special-case the equivalence of BDNAME
  // types here by truncating the larger, zero-padded name to its string
  // length and comparing against the shorter name.
  //
  // Note: it may be the case that both strings are zero-padded but that
  // hasn't come up yet so this implementation doesn't handle it.
  if (p1->type == BT_PROPERTY_BDNAME && p1->len != p2->len) {
    const bt_property_t *shorter = p1, *longer = p2;
    if (p1->len > p2->len) {
      shorter = p2;
      longer = p1;
    }
    return strlen((const char *)longer->val) == (size_t)shorter->len && !memcmp(longer->val, shorter->val, shorter->len);
  }

  return p1->len == p2->len && !memcmp(p1->val, p2->val, p1->len);
}

void property_free(bt_property_t *property) {
  property_free_array(property, 1);
}

void property_free_array(bt_property_t *properties, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    free(properties[i].val);
  }

  free(properties);
}
