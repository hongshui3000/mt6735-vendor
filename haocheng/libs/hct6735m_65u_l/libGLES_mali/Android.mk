LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libGLES_mali
LOCAL_SRC_FILES_64 := libGLES_mali.so
LOCAL_SRC_FILES_32 := arm/libGLES_mali.so
LOCAL_SHARED_LIBRARIES := libbinder libdpframework libged libgpu_aux libgralloc_extra libhardware libmtk_drvb libstdc++ libstlport libsync libui
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_PATH_32 := $($(TARGET_2ND_ARCH_VAR_PREFIX)TARGET_OUT_SHARED_LIBRARIES)/egl
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_SHARED_LIBRARIES)/egl
include $(BUILD_PREBUILT)
