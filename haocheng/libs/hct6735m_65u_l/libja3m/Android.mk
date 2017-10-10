LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libja3m
LOCAL_SRC_FILES_64 := libja3m.so
LOCAL_SRC_FILES_32 := arm/libja3m.so
LOCAL_SHARED_LIBRARIES := liba3m libandroid libstdc++ libstlport
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
