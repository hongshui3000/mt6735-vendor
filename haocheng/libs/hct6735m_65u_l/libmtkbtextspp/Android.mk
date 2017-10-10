LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libmtkbtextspp
LOCAL_SRC_FILES_32 := arm/libmtkbtextspp.so
LOCAL_SHARED_LIBRARIES := libbinder libbtstd libstdc++
LOCAL_MULTILIB := 32
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
