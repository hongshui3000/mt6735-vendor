LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libextdun_jni
LOCAL_SRC_FILES_32 := arm/libextdun_jni.so
LOCAL_SHARED_LIBRARIES := libbtstd libmtkbtextspp libstdc++
LOCAL_MULTILIB := 32
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
