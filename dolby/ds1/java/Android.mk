################################################################################
#  This program is protected under international and U.S. copyright laws as
#  an unpublished work. This program is confidential and proprietary to the
#  copyright owners. Reproduction or disclosure, in whole or in part, or the
#  production of derivative works therefrom without the express permission of
#  the copyright owners is prohibited.
#
#                 Copyright (C) 2012-2013 by Dolby Laboratories,
#                             All rights reserved.
################################################################################

ifeq ($(strip $(MTK_DOLBY_DAP_SUPPORT)), yes)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

DAP_JAVA_SUBDIRS := android/dolby
LOCAL_SRC_FILES := $(call find-other-java-files,$(DAP_JAVA_SUBDIRS))

LOCAL_SRC_FILES += \
	android/dolby/IDs.aidl \
	android/dolby/IDsServiceCallbacks.aidl

LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_JAVA_LIBRARIES := bouncycastle core core-junit ext framework framework2

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := dolby_ds
LOCAL_MODULE_CLASS := JAVA_LIBRARIES

LOCAL_NO_EMMA_INSTRUMENT := true
LOCAL_NO_EMMA_COMPILE := true

LOCAL_DX_FLAGS := --core-library

LOCAL_MODULE_OWNER := dolby
LOCAL_MODULE_PATH := $(TARGET_OUT_JAVA_LIBRARIES)

include $(BUILD_STATIC_JAVA_LIBRARY)
endif # DOLBY_DAP END
