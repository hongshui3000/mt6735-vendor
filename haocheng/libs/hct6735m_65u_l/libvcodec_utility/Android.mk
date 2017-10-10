LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libvcodec_utility
LOCAL_SRC_FILES_64 := libvcodec_utility.so
LOCAL_SRC_FILES_32 := arm/libvcodec_utility.so
LOCAL_SHARED_LIBRARIES := libbwc libion libion_mtk libm4u libmtk_drvb libstdc++
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)
