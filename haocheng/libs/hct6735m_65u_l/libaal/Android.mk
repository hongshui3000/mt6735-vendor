LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libaal
LOCAL_SRC_FILES_64 := libaal.so
LOCAL_SRC_FILES_32 := arm/libaal.so
LOCAL_SHARED_LIBRARIES := libbinder libgui libhardware libhardware_legacy libskia libstdc++ libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
