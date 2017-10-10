LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libdrmctaplugin
LOCAL_SRC_FILES_64 := libdrmctaplugin.so
LOCAL_SRC_FILES_32 := arm/libdrmctaplugin.so
LOCAL_SHARED_LIBRARIES := libandroid_runtime libbinder libdrmframework libdrmmtkutil libicui18n libicuuc libmtk_drvb libstdc++ libstlport
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib/drm
include $(BUILD_PREBUILT)
