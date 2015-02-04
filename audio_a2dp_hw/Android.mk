LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_HAVE_DYN_A2DP_SAMPLERATE),true)
LOCAL_CFLAGS += -DDYN_SAMPLERATE
else
ifeq ($(BOARD_USES_LEGACY_ALSA_AUDIO),true)
LOCAL_CFLAGS += -DSAMPLE_RATE_48K
endif
endif

LOCAL_SRC_FILES := \
	audio_a2dp_hw.c

LOCAL_C_INCLUDES += \
	. \
	$(LOCAL_PATH)/../utils/include

LOCAL_CFLAGS += -std=c99

LOCAL_MODULE := audio.a2dp.default
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_SHARED_LIBRARIES := libcutils liblog

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
