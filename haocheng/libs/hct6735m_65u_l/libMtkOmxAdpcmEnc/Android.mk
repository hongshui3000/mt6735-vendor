LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libMtkOmxAdpcmEnc
LOCAL_SRC_FILES_64 := libMtkOmxAdpcmEnc.so
LOCAL_SRC_FILES_32 := arm/libMtkOmxAdpcmEnc.so
LOCAL_SHARED_LIBRARIES := libbinder libmtk_drvb libstdc++ libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
