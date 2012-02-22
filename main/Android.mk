LOCAL_PATH:= $(call my-dir)

#
# Bluetooth HW module
# 

include $(CLEAR_VARS)

# HAL layer 
LOCAL_SRC_FILES:= \
	../btif/src/bluetooth.c\

# platform specific
LOCAL_SRC_FILES+= \
        bte_main.c \
	bte_init.c \
	bte_version.c \
	bte_logmsg.c\

# BTIF
LOCAL_SRC_FILES += \
	../btif/src/btif_core.c \
	../btif/src/btif_dm.c \
	../btif/src/btif_storage.c \
	../btif/src/btif_util.c \
	../btif/src/btif_hf.c \

# callouts
LOCAL_SRC_FILES+= \
	../btif/co/bta_sys_co.c \
	../btif/co/bta_fs_co.c \
	../btif/co/bta_ag_co.c \
	../btif/co/bta_dm_co.c \

# candidates for vendor lib (keep here for now)
LOCAL_SRC_FILES+= \
	../udrv/ulinux/unv_linux.c\


LOCAL_C_INCLUDES+= . \
                   $(LOCAL_PATH)/../bta/include \
                   $(LOCAL_PATH)/../bta/sys \
                   $(LOCAL_PATH)/../bta/dm \
                   $(LOCAL_PATH)/../gki/common \
                   $(LOCAL_PATH)/../gki/ulinux \
                   $(LOCAL_PATH)/../include \
                   $(LOCAL_PATH)/../stack/include \
                   $(LOCAL_PATH)/../stack/l2cap \
                   $(LOCAL_PATH)/../stack/btm \
                   $(LOCAL_PATH)/../hcis \
                   $(LOCAL_PATH)/../hcis/include \
                   $(LOCAL_PATH)/../hcis/patchram \
                   $(LOCAL_PATH)/../udrv/include \
                   $(LOCAL_PATH)/../btif/include \
                   $(LOCAL_PATH)/../vendor/libvendor/include\

LOCAL_CFLAGS += -DBUILDCFG -Werror

ifeq ($(TARGET_PRODUCT), full_crespo)
     LOCAL_CFLAGS += -DTARGET_CRESPO
else
     LOCAL_CFLAGS += -DTARGET_MAGURO
endif

# Fix this
#ifeq ($(TARGET_VARIANT), eng)
#     LOCAL_CFLAGS += -O2 # and other production release flags
#else
#     LOCAL_CFLAGS +=
#endif

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libpower \
    libbt-vendor

#LOCAL_WHOLE_STATIC_LIBRARIES := libbt-brcm_gki libbt-brcm_stack libbt-brcm_bta
LOCAL_STATIC_LIBRARIES := libbt-brcm_gki libbt-brcm_stack libbt-brcm_bta

LOCAL_MODULE := bluetooth.default
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := eng


include $(BUILD_SHARED_LIBRARY)
