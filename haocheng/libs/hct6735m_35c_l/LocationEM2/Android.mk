LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := LocationEM2
LOCAL_SRC_FILES := LocationEM2.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := .apk
LOCAL_CERTIFICATE := platform
include $(BUILD_PREBUILT)
