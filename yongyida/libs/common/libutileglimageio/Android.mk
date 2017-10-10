LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libutileglimageio
LOCAL_SRC_FILES_64 := libutileglimageio.a
LOCAL_SRC_FILES_32 := arm/libutileglimageio.a
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
