################################################################################
#  This program is protected under international and U.S. copyright laws as
#  an unpublished work. This program is confidential and proprietary to the
#  copyright owners. Reproduction or disclosure, in whole or in part, or the
#  production of derivative works therefrom without the express permission of
#  the copyright owners is prohibited.
#
#                 Copyright (C) 2011-2013 by Dolby Laboratories,
#                             All rights reserved.
################################################################################

#DOLBY_UDC
ifeq ($(strip $(MTK_AUDIO_DDPLUS_SUPPORT)), yes)

LOCAL_PATH:= $(call my-dir)
MYLOCAL_PATH := $(LOCAL_PATH)
include $(CLEAR_VARS)

ifneq (,$(findstring $(LOCAL_PATH)/prebuilt,$(wildcard $(LOCAL_PATH)/prebuilt)))

# Tell the build system about the existing (pre-built) DD+ library.

# Note: The library name appears here and in the LOCAL_WHOLE_STATIC_LIBRARIES below.
# Also, the path is relative to the path of this makefile
LOCAL_PREBUILT_LIBS := prebuilt/udc/bin/udc_lib.a

include $(BUILD_MULTI_PREBUILT)

###############################################################################

LOCAL_PATH := $(MYLOCAL_PATH)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := DDPDecoder.cpp src/ddpdec_client.c src/udc_user.c

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        frameworks/av/include/media/stagefright \
        frameworks/native/include/media/openmax \
        $(LOCAL_PATH)/src \
        $(LOCAL_PATH)/prebuilt/udc/include

LOCAL_CFLAGS := -DOSCL_UNUSED_ARG= -DOSCL_IMPORT_REF= -DDOLBY_UDC

LOCAL_STATIC_LIBRARIES := udc_lib

#DOLBY_MONO_SPEAKER
ifdef DOLBY_MONO_SPEAKER
  LOCAL_CFLAGS += -DDOLBY_MONO_SPEAKER
endif
#DOLBY_MONO_SPEAKER_END

LOCAL_CFLAGS += -DBYTES_PER_UDC_SAMPLE=2
#for 24bit file BYTES_PER_UDC_SAMPLE=4
#LOCAL_CFLAGS += -DBYTES_PER_UDC_SAMPLE=4

# log level: 0=off, 1=informational, 2=intense, 3=verbose.
LOCAL_CFLAGS += -DDOLBY_UDC_LOG_LEVEL=1

LOCAL_SHARED_LIBRARIES := libstagefright libstagefright_omx libstagefright_foundation libutils libcutils

LOCAL_MODULE := libstagefright_soft_ddpdec

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_OWNER := dolby

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

include $(BUILD_SHARED_LIBRARY)

else

LOCAL_SRC_FILES := libstagefright_soft_ddpdec.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)
LOCAL_MODULE := libstagefright_soft_ddpdec
include $(BUILD_PREBUILT)

endif

endif
#DOLBY_UDC_END
