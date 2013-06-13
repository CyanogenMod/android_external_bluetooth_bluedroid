LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -DBUILDCFG $(bdroid_CFLAGS)

LOCAL_SRC_FILES := src/btc_common.c

LOCAL_MODULE := libbt-btc
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils libc

LOCAL_C_INCLUDES := . \
                   $(LOCAL_PATH)/include \
                   $(LOCAL_PATH)/../gki/common \
                   $(LOCAL_PATH)/../gki/ulinux \
                   $(LOCAL_PATH)/../include \
                   $(LOCAL_PATH)/../stack/include \
                   $(LOCAL_PATH)/../hci/include \
                   $(LOCAL_PATH)/../stack/btm \
                   $(LOCAL_PATH)/../udrv/include \
                   $(LOCAL_PATH)/../bta/include \
                   $(bdroid_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)
