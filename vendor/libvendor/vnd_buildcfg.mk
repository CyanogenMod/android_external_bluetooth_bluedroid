intermediates := $(local-intermediates-dir)

SRC := $(call my-dir)/include/$(addprefix vnd_, $(addsuffix .txt,$(basename $(TARGET_DEVICE))))
GEN := $(intermediates)/vnd_buildcfg.h
TOOL := $(call my-dir)/../../tools/gen-buildcfg.sh

$(GEN): PRIVATE_PATH := $(call my-dir)
$(GEN): PRIVATE_CUSTOM_TOOL = $(TOOL) $< $@
$(GEN): $(SRC)  $(TOOL)
	$(transform-generated-source)

LOCAL_GENERATED_SOURCES += $(GEN)
