LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := hwcomposer.mt6735m.1.4.1
LOCAL_SRC_FILES_64 := hwcomposer.mt6735m.1.4.1.a
LOCAL_SRC_FILES_32 := arm/hwcomposer.mt6735m.1.4.1.a
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
