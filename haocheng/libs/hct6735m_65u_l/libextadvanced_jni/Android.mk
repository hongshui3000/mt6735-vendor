LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libextadvanced_jni
LOCAL_SRC_FILES_64 := libextadvanced_jni.so
LOCAL_SRC_FILES_32 := arm/libextadvanced_jni.so
LOCAL_SHARED_LIBRARIES := libbtstd libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
