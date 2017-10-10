LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libc_malloc_debug_mtk
LOCAL_SRC_FILES_64 := libc_malloc_debug_mtk.a
LOCAL_SRC_FILES_32 := arm/libc_malloc_debug_mtk.a
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
