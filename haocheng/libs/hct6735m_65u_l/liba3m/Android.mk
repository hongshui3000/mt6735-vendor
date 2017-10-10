LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := liba3m
LOCAL_SRC_FILES_64 := liba3m.so
LOCAL_SRC_FILES_32 := arm/liba3m.so
LOCAL_SHARED_LIBRARIES := libEGL libGLESv2 libandroid libmtk_drvb libstdc++ libstlport
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
