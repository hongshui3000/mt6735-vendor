LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libstagefrighthw
LOCAL_SRC_FILES_64 := libstagefrighthw.so
LOCAL_SRC_FILES_32 := arm/libstagefrighthw.so
LOCAL_SHARED_LIBRARIES := libbinder libstagefright_foundation libstdc++ libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
