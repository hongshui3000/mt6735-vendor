LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libextem
LOCAL_SRC_FILES_64 := libextem.so
LOCAL_SRC_FILES_32 := arm/libextem.so
LOCAL_SHARED_LIBRARIES := libbtstd libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
