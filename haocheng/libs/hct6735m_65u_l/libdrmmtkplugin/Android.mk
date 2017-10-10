LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libdrmmtkplugin
LOCAL_SRC_FILES_64 := libdrmmtkplugin.so
LOCAL_SRC_FILES_32 := arm/libdrmmtkplugin.so
LOCAL_SHARED_LIBRARIES := libdrmmtkutil libicui18n libicuuc libmtk_drvb libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib/drm
include $(BUILD_PREBUILT)
