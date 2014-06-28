LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += $(bdroid_CFLAGS)

LOCAL_SRC_FILES := \
	src/bt_hci_bdroid.c \
	src/btsnoop.c \
	src/btsnoop_net.c \
	src/lpm.c \
	src/utils.c \
	src/vendor.c

LOCAL_CFLAGS := -Wno-unused-parameter

ifeq ($(BLUETOOTH_HCI_USE_MCT),true)

LOCAL_CFLAGS += -DHCI_USE_MCT

LOCAL_SRC_FILES += \
	src/hci_mct.c \
	src/userial_mct.c

else
LOCAL_SRC_FILES += \
	src/hci_h4.c \
	src/userial.c
endif

LOCAL_CFLAGS += -std=c99

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../osi/include \
	$(LOCAL_PATH)/../utils/include \
        $(bdroid_C_INCLUDES)

LOCAL_MODULE := libbt-hci
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

include $(BUILD_STATIC_LIBRARY)
