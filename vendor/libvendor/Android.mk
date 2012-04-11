LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        src/bt_vendor_brcm.c \
        src/hci_h4.c \
        src/userial.c \
        src/hardware.c \
        src/upio.c \
        src/utils.c \
        src/btsnoop.c \
        src/conf.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include
		   
LOCAL_SHARED_LIBRARIES := \
        libcutils

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)
