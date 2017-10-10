LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PREBUILT_EXECUTABLES := calib.dat
include $(BUILD_MULTI_PREBUILT)
