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

#pragma once

#include "base.h"

bt_property_t *property_copy_array(const bt_property_t *properties, size_t count);
bt_property_t *property_new_name(const char *name);
bt_property_t *property_new_discovery_timeout(uint32_t timeout);

const char *property_extract_name(const bt_property_t *property);

bool property_equals(const bt_property_t *p1, const bt_property_t *p2);

void property_free(bt_property_t *property);
void property_free_array(bt_property_t *properties, size_t count);
