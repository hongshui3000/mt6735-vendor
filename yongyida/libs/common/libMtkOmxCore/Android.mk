LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libMtkOmxCore
LOCAL_SRC_FILES_64 := libMtkOmxCore.so
LOCAL_SRC_FILES_32 := arm/libMtkOmxCore.so
LOCAL_SHARED_LIBRARIES := libbinder libstdc++ libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
