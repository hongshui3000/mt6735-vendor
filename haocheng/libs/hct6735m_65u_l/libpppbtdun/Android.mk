LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libpppbtdun
LOCAL_SRC_FILES_64 := libpppbtdun.so
LOCAL_SRC_FILES_32 := arm/libpppbtdun.so
LOCAL_SHARED_LIBRARIES := libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
