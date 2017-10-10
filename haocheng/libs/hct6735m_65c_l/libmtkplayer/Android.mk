LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libmtkplayer
LOCAL_SRC_FILES_64 := libmtkplayer.so
LOCAL_SRC_FILES_32 := arm/libmtkplayer.so
LOCAL_SHARED_LIBRARIES := libaudiosetting libbinder libmedia libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
