LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libMtkOmxRawDec
LOCAL_SRC_FILES_64 := libMtkOmxRawDec.so
LOCAL_SRC_FILES_32 := arm/libMtkOmxRawDec.so
LOCAL_SHARED_LIBRARIES := libbinder libstdc++ libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
