LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libfacebeauty
LOCAL_SRC_FILES_64 := libfacebeauty.a
LOCAL_SRC_FILES_32 := arm/libfacebeauty.a
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
