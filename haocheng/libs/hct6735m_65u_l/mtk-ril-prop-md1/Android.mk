LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := mtk-ril-prop-md1
LOCAL_SRC_FILES_64 := mtk-ril-prop-md1.a
LOCAL_SRC_FILES_32 := arm/mtk-ril-prop-md1.a
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
