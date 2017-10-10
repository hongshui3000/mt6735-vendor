LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libgpu_aux
LOCAL_SRC_FILES_64 := libgpu_aux.so
LOCAL_SRC_FILES_32 := arm/libgpu_aux.so
LOCAL_SHARED_LIBRARIES := libdpframework libgralloc_extra libhardware libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
