LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libyamaha_utils
LOCAL_SRC_FILES_64 := libyamaha_utils.a
LOCAL_MULTILIB := 64
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
