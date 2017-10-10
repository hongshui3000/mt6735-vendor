LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libbtstd
LOCAL_SRC_FILES_64 := libbtstd.so
LOCAL_SRC_FILES_32 := arm/libbtstd.so
LOCAL_SHARED_LIBRARIES := libbinder libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
