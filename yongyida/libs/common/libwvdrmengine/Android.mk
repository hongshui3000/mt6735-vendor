LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libwvdrmengine
LOCAL_SRC_FILES_32 := arm/libwvdrmengine.so
LOCAL_SHARED_LIBRARIES := libcrypto libstagefright_foundation libstdc++ libstlport
LOCAL_MULTILIB := 32
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_PATH := $(TARGET_OUT)/vendor/lib/mediadrm
include $(BUILD_PREBUILT)
