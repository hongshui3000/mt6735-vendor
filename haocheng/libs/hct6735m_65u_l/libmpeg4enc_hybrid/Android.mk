LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libmpeg4enc_hybrid
LOCAL_SRC_FILES_32 := arm/libmpeg4enc_hybrid.a
LOCAL_MULTILIB := 32
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
