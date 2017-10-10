LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libnvramagentclient
LOCAL_SRC_FILES_64 := libnvramagentclient.so
LOCAL_SRC_FILES_32 := arm/libnvramagentclient.so
LOCAL_SHARED_LIBRARIES := libbinder libnvram libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
