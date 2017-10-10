LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libvie
LOCAL_SRC_FILES_32 := arm/libvie.so
LOCAL_SHARED_LIBRARIES := libgui libmedia libmtk_drvb libsqlite libstagefright libstdc++ libstlport
LOCAL_MULTILIB := 32
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
