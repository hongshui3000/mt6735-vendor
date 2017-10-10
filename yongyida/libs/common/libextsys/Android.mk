LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libextsys
LOCAL_SRC_FILES_64 := libextsys.so
LOCAL_SRC_FILES_32 := arm/libextsys.so
LOCAL_SHARED_LIBRARIES := libbinder libbtstd libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
