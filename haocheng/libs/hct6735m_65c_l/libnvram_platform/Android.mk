LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libnvram_platform
LOCAL_SRC_FILES_64 := libnvram_platform.so
LOCAL_SRC_FILES_32 := arm/libnvram_platform.so
LOCAL_SHARED_LIBRARIES := libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
