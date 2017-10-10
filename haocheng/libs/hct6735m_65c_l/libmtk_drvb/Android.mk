LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libmtk_drvb
LOCAL_SRC_FILES_64 := libmtk_drvb.so
LOCAL_SRC_FILES_32 := arm/libmtk_drvb.so
LOCAL_SHARED_LIBRARIES := libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
