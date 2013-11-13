LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        src/bt_hci_bdroid.c \
        src/lpm.c \
        src/bt_hw.c \
        src/btsnoop.c \
        src/utils.c

LOCAL_SRC_FILES += \
        src/userial.c \
        src/userial_mct.c \
        src/hci_mct.c \
        src/hci_h4.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../utils/include

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libdl \
        libbt-utils

LOCAL_MODULE := libbt-hci
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)
