# Copyright (C) 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

#LOCAL_COMMON_PATH:=common

LOCAL_COMMON_PATH:=../../../common

# HAL module implemenation stored in
# hw/<POWERS_HARDWARE_MODULE_ID>.<ro.hardware>.so
include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_C_INCLUDES += hardware/libhardware/include \
                    vendor/mediatek/proprietary/hardware/libmemtrack

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_SRC_FILES += memtrack_mali_midgard.c

LOCAL_MODULE := libmemtrack_GL

include $(BUILD_STATIC_LIBRARY)
