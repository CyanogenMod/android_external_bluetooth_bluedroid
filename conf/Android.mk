LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := bt_stack.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/bluetooth
LOCAL_MODULE_TAGS := eng
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := bt_did.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/bluetooth
LOCAL_MODULE_TAGS := eng
LOCAL_SRC_FILES :=  $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := auto_pair_devlist.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/bluetooth
LOCAL_MODULE_TAGS := eng
LOCAL_SRC_FILES :=  $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(TARGET_PRODUCT), full_maguro)
    include $(LOCAL_PATH)/samsung/maguro/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_crespo)
    include $(LOCAL_PATH)/samsung/crespo/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_crespo4g)
    include $(LOCAL_PATH)/samsung/crespo4g/Android.mk
endif
ifeq ($(TARGET_PRODUCT), full_wingray)
    include $(LOCAL_PATH)/moto/wingray/Android.mk
endif

