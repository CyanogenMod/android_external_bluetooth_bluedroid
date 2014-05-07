#
#  Copyright (C) 2014 Google, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at:
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := bdtest

LOCAL_SRC_FILES := \
	cases/adapter.c \
	cases/cases.c \
	cases/pan.c \
	support/adapter.c \
	support/callbacks.c \
	support/hal.c \
	support/pan.c \
	support/property.c \
	main.c

LOCAL_SHARED_LIBRARIES += \
	libhardware \
	libhardware_legacy

LOCAL_CFLAGS += -std=c99 -Wall -Wno-unused-parameter -Wno-missing-field-initializers -Werror

LOCAL_MULTILIB := 32

include $(BUILD_EXECUTABLE)
