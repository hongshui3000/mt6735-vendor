LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libtouchfilter
LOCAL_SRC_FILES_64 := libtouchfilter.so
LOCAL_SRC_FILES_32 := arm/libtouchfilter.so
LOCAL_SHARED_LIBRARIES := libinput libmtk_drvb libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
