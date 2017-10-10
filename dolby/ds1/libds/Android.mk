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

ifeq ($(strip $(MTK_DOLBY_DAP_SUPPORT)), yes)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifneq (,$(findstring $(LOCAL_PATH)/prebuilt,$(wildcard $(LOCAL_PATH)/prebuilt)))

MY_LOCAL_PATH:=$(LOCAL_PATH)

# Pre-built DS library
LOCAL_PREBUILT_LIBS := \
    ./prebuilt/ds/bin/libak_dap.a \
    ./prebuilt/ds/bin/dlb_buffer.a \
    ./prebuilt/ds/bin/dvlim_dap.a \
    ./prebuilt/ds/bin/dlb_intrinsics.a \
    ./prebuilt/ds/bin/chital_dap.a

LOCAL_CFLAGS += -DDOLBY_DAP
LOCAL_CFLAGS += -DDOLBY_DAP_OPENSLES

include $(BUILD_MULTI_PREBUILT)

LOCAL_PATH:=$(MY_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= EffectDs.cpp \
    util/Ds1apBuffer.c \
    util/Ds1ap.cpp \
    util/ak_process_loop.c

LOCAL_CFLAGS += -O3 -DDOLBY_DAP_OPENSLES -DUSE_AKBUFFER -DUSE_CROSSFADE
# DS audio effect log level: 0=off, 1=informational, 2=intense, 3=verbose.
LOCAL_CFLAGS += -DDOLBY_DAP_LOG_LEVEL=1
# DS audio effect compilation switch for applying the pregain.
# Note: Keep this definition consistent with Android.mk in AudioFlinger
LOCAL_CFLAGS += -DDOLBY_DAP_OPENSLES_PREGAIN
# DAP compilation switch to suspend DAP if system sound is present
LOCAL_CFLAGS += -DDOLBY_DAP_BYPASS_SOUND_TYPES
ifeq ($(strip $(MTK_HIGH_RESOLUTION_AUDIO_SUPPORT)),yes)
LOCAL_CFLAGS += -DDOLBY_DAP_32BIT
endif
#DOLBY_DAP_OPENSLES_LPA
ifdef DOLBY_DAP_OPENSLES_LPA
    LOCAL_CFLAGS += -DDOLBY_DAP_OPENSLES_LPA
endif
#DOLBY_DAP_OPENSLES_LPA_END

LOCAL_STATIC_LIBRARIES += \
    libak_dap \
    dlb_buffer \
    dvlim_dap \
    dlb_intrinsics \
    chital_dap

LOCAL_SHARED_LIBRARIES := \
     libcutils \
     liblog \
     libutils

LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/soundfx

LOCAL_MODULE:= libdseffect

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_OWNER := dolby

ifeq ($(TARGET_OS)-$(TARGET_SIMULATOR),linux-true)
LOCAL_LDLIBS += -ldl
endif

ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libdl
endif


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/prebuilt/ds/include \
	$(LOCAL_PATH)/util \
	$(LOCAL_PATH)/include \
	$(call include-path-for, audio-effects)
	
LOCAL_PRELINK_MODULE := false

LOCAL_LDFLAGS := -Wl,--no-fatal-warnings

include $(BUILD_SHARED_LIBRARY)

endif
#DOLBY_DAP_OPENSLES_END

endif
#DOLBY_DAP_END
