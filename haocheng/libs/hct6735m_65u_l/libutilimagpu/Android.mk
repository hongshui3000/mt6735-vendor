LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libutilimagpu
LOCAL_SRC_FILES_64 := libutilimagpu.a
LOCAL_SRC_FILES_32 := arm/libutilimagpu.a
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
