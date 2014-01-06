LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= sdptool.c

LOCAL_C_INCLUDES += . \
        $(LOCAL_PATH)/../../stack/include \
        $(LOCAL_PATH)/../../include \
        $(LOCAL_PATH)/../../stack/l2cap \
        $(LOCAL_PATH)/../../gki/ulinux \
        $(LOCAL_PATH)/../../utils/include \
        $(LOCAL_PATH)/btif/include \
        $(bdroid_C_INCLUDES)

LOCAL_CFLAGS += $(bdroid_CFLAGS)
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := debug optional
LOCAL_MODULE:= sdptool

LOCAL_LDLIBS += -lpthread -ldl -llog

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libhardware \
                          libhardware_legacy

include $(BUILD_EXECUTABLE)
