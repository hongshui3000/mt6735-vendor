LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libMtkOmxAlacDec
LOCAL_SRC_FILES_64 := libMtkOmxAlacDec.so
LOCAL_SRC_FILES_32 := arm/libMtkOmxAlacDec.so
LOCAL_SHARED_LIBRARIES := libbinder libmtk_drvb libstdc++ libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
