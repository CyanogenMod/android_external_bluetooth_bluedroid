LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += $(bdroid_CFLAGS)

LOCAL_SRC_FILES := \
        src/bt_hci_bdroid.c \
        src/lpm.c \
        src/bt_hw.c \
        src/btsnoop.c \
        src/utils.c

ifeq ($(QCOM_BT_USE_SMD_TTY),true)
LOCAL_CFLAGS += -DQCOM_WCN_SSR
endif

ifeq ($(BLUETOOTH_HCI_USE_USB),true)

LOCAL_CFLAGS += -DHCI_H2

LOCAL_SRC_FILES += \
        src/usb.c \
        src/hci_h4.c

LOCAL_C_INCLUDES += \
        external/libusb

LOCAL_SHARED_LIBRARIES := \
        libusb

else

LOCAL_SRC_FILES += \
        src/userial.c \
        src/hci_h4.c

endif

LOCAL_SRC_FILES += \
        src/userial_mct.c \
        src/hci_mct.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(LOCAL_PATH)/../utils/include \
        $(bdroid_C_INCLUDES)

LOCAL_SHARED_LIBRARIES += \
        libcutils \
        liblog \
        libdl \
        libbt-utils

LOCAL_MODULE := libbt-hci
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

include $(BUILD_SHARED_LIBRARY)
