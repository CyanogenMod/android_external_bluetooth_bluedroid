LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := bcm4330.hcd
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/firmware

#LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_SRC_FILES := BCM4330B1.hcd

#PRODUCT_COPY_FILES += \
#    hardware/broadcom/bt/firmware/bcm4330/BCM4330B1.hcd:system/vendor/firmware/BCM4330B1.hcd

LOCAL_MODULE_TAGS := eng

include $(BUILD_PREBUILT)


