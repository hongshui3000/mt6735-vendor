LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libnvram_sec
LOCAL_SRC_FILES_64 := libnvram_sec.so
LOCAL_SRC_FILES_32 := arm/libnvram_sec.so
LOCAL_SHARED_LIBRARIES := libcustom_nvram libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
