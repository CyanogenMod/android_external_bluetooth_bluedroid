LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    ./src/config.c \
    ./src/fixed_queue.c \
    ./src/list.c \
    ./src/semaphore.c

LOCAL_CFLAGS := -std=c99 -Wall -Werror
LOCAL_MODULE := libosi
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libc liblog
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

include $(BUILD_STATIC_LIBRARY)

#####################################################

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    ./test/config_test.cpp \
    ./test/list_test.cpp

LOCAL_CFLAGS := -Wall -Werror
LOCAL_MODULE := ositests
LOCAL_MODULE_TAGS := tests
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_STATIC_LIBRARIES := libosi

include $(BUILD_NATIVE_TEST)
