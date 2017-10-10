LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libnativecheck-jni
LOCAL_SRC_FILES_64 := libnativecheck-jni.so
LOCAL_SRC_FILES_32 := arm/libnativecheck-jni.so
LOCAL_SHARED_LIBRARIES := libandroid_runtime libmtk_drvb libnativehelper libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
