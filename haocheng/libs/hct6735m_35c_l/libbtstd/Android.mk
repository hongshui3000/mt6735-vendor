LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libbtstd
LOCAL_SRC_FILES := libbtstd.so
LOCAL_SHARED_LIBRARIES := libbinder libstdc++
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
