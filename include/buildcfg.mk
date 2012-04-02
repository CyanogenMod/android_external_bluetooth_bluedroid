#bt_targetfile = $(TARGET_DEVICE_DIR)/$(addprefix bdroid_, $(addsuffix .txt,$(basename $(TARGET_DEVICE))))
bt_targetfile = $(call my-dir)/$(addprefix bdroid_, $(addsuffix .txt,$(basename $(TARGET_DEVICE))))
bt_cfgfile    = $(call my-dir)/buildcfg.h

bt_build_cfg = $(shell if [ -f $(bt_cfgfile) ] && [ `stat -c %Y $(bt_targetfile)` -lt `stat -c %Y $(bt_cfgfile)` ]; then echo 0; else echo 1; fi)

ifeq ($(bt_build_cfg),1)
$(info "Creating $(bt_cfgfile) from $(bt_targetfile)")
$(shell echo "#ifndef BUILDCFG_H" > $(bt_cfgfile))
$(shell echo "#define BUILDCFG_H" >> $(bt_cfgfile))
$(shell sed -e '/^#/d' -e '/^$$/d' -e '/# Makefile only$$/d' -e 's/^/#define /' -e 's/=/ /' $(bt_targetfile) >> $(bt_cfgfile))
$(shell echo "#endif" >> $(bt_cfgfile))
endif
