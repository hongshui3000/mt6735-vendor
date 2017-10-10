LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libmtk-art-runtime
LOCAL_SRC_FILES_64 := libmtk-art-runtime.a
LOCAL_SRC_FILES_32 := arm/libmtk-art-runtime.a
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
