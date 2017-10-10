LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libexttestmode
LOCAL_SRC_FILES_32 := arm/libexttestmode.so
LOCAL_SHARED_LIBRARIES := libbtcust libbtcusttc1 libbtstd libextsys libstdc++
LOCAL_MULTILIB := 32
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
