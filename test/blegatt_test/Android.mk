LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)


LOCAL_SRC_FILES:= gatt_test.c

LOCAL_C_INCLUDES := . \
         $(LOCAL_PATH)/../../stack/include \
         $(LOCAL_PATH)/../../include \
         $(LOCAL_PATH)/../../stack/l2cap \
         $(LOCAL_PATH)/../../stack/gatt \
         $(LOCAL_PATH)/../../stack/a2dp \
         $(LOCAL_PATH)/../../stack/btm \
         $(LOCAL_PATH)/../../stack/avdt \
         $(LOCAL_PATH)/../../stack/btm \
         $(LOCAL_PATH)/../../gki/common \
         $(LOCAL_PATH)/../../gki/ulinux \
         $(LOCAL_PATH)/../../udrv/include \
         $(LOCAL_PATH)/../../rpc/include \
         $(LOCAL_PATH)/../../hcis \
         $(LOCAL_PATH)/../../hci/include \
         $(LOCAL_PATH)/../../ctrlr/include \
         $(LOCAL_PATH)/../../bta/include \
         $(LOCAL_PATH)/../../bta/sys \
         $(LOCAL_PATH)/../../brcm/include \
         $(LOCAL_PATH)/../../utils/include \
         $(LOCAL_PATH)/btif/include \
         $(LOCAL_PATH)/embdrv/sbc/encoder/include \
         $(bdroid_C_INCLUDES) \
         external/tinyxml2

LOCAL_CFLAGS += -std=c99

LOCAL_CFLAGS += $(bdroid_CFLAGS)
LOCAL_MODULE_TAGS := debug optional

LOCAL_MODULE:= gatt_testtool

LOCAL_LDLIBS +=  -ldl -llog
LIBS_c += -lreadline

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libhardware \
                          libhardware_legacy

LOCAL_MULTILIB := 32

include $(BUILD_EXECUTABLE)
