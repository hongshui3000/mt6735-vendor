LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libjtranscode
LOCAL_SRC_FILES_32 := arm/libjtranscode.so
LOCAL_SHARED_LIBRARIES := libmtk_drvb libnativehelper libstdc++
LOCAL_MULTILIB := 32
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
