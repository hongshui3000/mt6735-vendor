LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= xlog_switch.c

LOCAL_MODULE:= libxlog_switch
LOCAL_MODULE_TAGS := optional

ifeq ($(HAVE_XLOG_FEATURE), yes)
    LOCAL_CFLAGS += -DHAVE_XLOG_FEATURE
endif

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= xlog_switch.c

LOCAL_MODULE:= libxlog_switch
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
ifeq ($(HAVE_XLOG_FEATURE), yes)
    LOCAL_CFLAGS += -DHAVE_XLOG_FEATURE
endif

include $(BUILD_HOST_STATIC_LIBRARY)

include $(CLEAR_VARS)

mtklog_config_prop_file := $(TARGET_OUT)/etc/mtklog-config.prop
ifeq ($(TARGET_BUILD_VARIANT),eng)
mtklog_config_prop_src := $(LOCAL_PATH)/mtklog-config-eng.prop
else
ifeq ($(MTK_CHIPTEST_INT),yes)
mtklog_config_prop_src := $(LOCAL_PATH)/mtklog-config-chiptest.prop
else
mtklog_config_prop_src := $(LOCAL_PATH)/mtklog-config-user.prop
endif
endif

$(mtklog_config_prop_file): PRIVATE_SRC_FILES := $(mtklog_config_prop_src)
$(mtklog_config_prop_file): $(mtklog_config_prop_src) | $(ACP)
	mkdir -p $(dir $@)
	$(ACP) $(PRIVATE_SRC_FILES) $(mtklog_config_prop_file)

xlog_filter_tags_file := $(TARGET_OUT)/etc/xlog-filter-tags
xlog_filter_default_file := $(TARGET_OUT)/etc/xlog-filter-default
xlog_filter_tags_src := $(LOCAL_PATH)/tags-default.xlog $(LOCAL_PATH)/tags-setting.xlog

$(xlog_filter_tags_file): PRIVATE_SRC_FILES := $(xlog_filter_tags_src)
$(xlog_filter_tags_file): $(xlog_filter_tags_src)
	mkdir -p $(dir $@)
	$(MTK_ROOT)/external/xlog/tools/merge-xlog-filter-tags.py -t $@ $(PRIVATE_SRC_FILES)

$(xlog_filter_default_file): PRIVATE_SRC_FILES := $(xlog_filter_tags_src)
ifeq ($(OPTR_SPEC_SEG_DEF),OP03_SPEC0200_SEGDEFAULT)
$(xlog_filter_default_file): PRIVATE_XLOG_FLAGS := -l info
endif
$(xlog_filter_default_file): $(xlog_filter_tags_src)
	mkdir -p $(dir $@)
	$(MTK_ROOT)/external/xlog/tools/merge-xlog-filter-tags.py $(PRIVATE_XLOG_FLAGS) -f $@ $(PRIVATE_SRC_FILES)

ALL_DEFAULT_INSTALLED_MODULES += $(xlog_filter_tags_file) $(xlog_filter_default_file) $(mtklog_config_prop_file)
