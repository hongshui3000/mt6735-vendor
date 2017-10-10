LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libextsys_jni
LOCAL_SRC_FILES_64 := libextsys_jni.so
LOCAL_SRC_FILES_32 := arm/libextsys_jni.so
LOCAL_SHARED_LIBRARIES := libbtstd libextsys libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
