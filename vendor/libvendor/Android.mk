LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

vnd_targetfile = $(LOCAL_PATH)/include/$(addprefix vnd_, $(addsuffix .txt,$(basename $(TARGET_DEVICE))))
vnd_cfgfile    = $(LOCAL_PATH)/include/vnd_buildcfg.h

vnd_build_cfg = $(shell if [ -f $(vnd_cfgfile) ] && [ `stat -c %Y $(vnd_targetfile)` -lt `stat -c %Y $(vnd_cfgfile)` ]; then echo 0; else echo 1; fi)

ifeq ($(vnd_build_cfg),1)
$(info "Creating $(vnd_cfgfile) from $(vnd_targetfile)")
$(shell echo "#ifndef VND_BUILDCFG_H" > $(vnd_cfgfile))
$(shell echo "#define VND_BUILDCFG_H" >> $(vnd_cfgfile))
$(shell sed -e '/^#/d' -e '/^$$/d' -e '/# Makefile only$$/d' -e 's/^/#define /' -e 's/=/ /' $(vnd_targetfile) >> $(vnd_cfgfile))
$(shell echo "#endif" >> $(vnd_cfgfile))
endif

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

include $(BUILD_SHARED_LIBRARY)
