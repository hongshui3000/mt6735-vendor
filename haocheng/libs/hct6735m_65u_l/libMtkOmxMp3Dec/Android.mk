LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libMtkOmxMp3Dec
LOCAL_SRC_FILES_64 := libMtkOmxMp3Dec.so
LOCAL_SRC_FILES_32 := arm/libMtkOmxMp3Dec.so
LOCAL_SHARED_LIBRARIES := libbinder libmtk_drvb libstdc++ libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
