#
# libimageio_plat_drv_FrmB
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#
LOCAL_SRC_FILES := \
    cam/cam_path_FrmB.cpp \
    cam/cam_path_pass1_FrmB.cpp \
    cam/cam_path_pass2_FrmB.cpp \
    cam/p1HwCfg_FrmB.cpp \
    cam/isp_function_FrmB.cpp \
    cdp/cdp_drv_FrmB.cpp \
    mdp/mdp_mgr_FrmB.cpp

# camutils: For CameraProfile APIs.
#
LOCAL_C_INCLUDES := $(TOP)/bionic \
    $(TOP)/external/stlport/stlport \
    $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam \
    $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/D2/core/imageio_FrmB/inc \
    $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/D2/core/imageio_common/inc \
    $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/D2/core/imageio_FrmB/drv/inc \
    $(TOP)/$(MTK_PATH_PLATFORM)/kernel/core/include/mach \
    $(TOP)/$(MTK_PATH_PLATFORM)/external/ldvt/include \
    $(TOP)/$(MTK_PATH_PLATFORM)/hardware/m4u \
    $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/inc/common/camutils \
    $(TOP)/$(MTK_PATH_SOURCE)/hardware/dpframework/inc \
    $(TOP)/$(MTK_PATH_PLATFORM)/hardware/include/mtkcam/drv \
    $(TOP)/bionic/libc/kernel/common/linux/mt6582

#for camera_vendor_tags.h:
LOCAL_C_INCLUDES += $(TOP)/system/media/camera/include

# Add a define value that can be used in the code to indicate that it's using LDVT now.
# For print message function when using LDVT.
# Note: It will be cleared by "CLEAR_VARS", so if it is needed in other module, it
# has to be set in other module again.
ifeq ($(BUILD_MTK_LDVT),true)
    LOCAL_CFLAGS += -DUSING_MTK_LDVT
endif

#
LOCAL_STATIC_LIBRARIES := \

#
LOCAL_WHOLE_STATIC_LIBRARIES := \

#ifeq ($(BUILD_MTK_LDVT),true)
#    LOCAL_WHOLE_STATIC_LIBRARIES += libuvvf
#endif

LOCAL_SHARED_LIBRARIES := \
    libdpframework \
    libstlport \
    libcutils \
    libcamdrv_FrmB \
    libm4u \

#new from Jonas requests 130510
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_CFLAGS += -DUSING_D2
#
LOCAL_MODULE := libimageio_plat_drv_FrmB
LOCAL_MODULE_TAGS := optional
#LOCAL_MULTILIB := 32
#
#include $(BUILD_STATIC_LIBRARY)

#
# Start of common part ------------------------------------
-include $(TOP)/$(MTK_PATH_PLATFORM)/hardware/mtkcam/mtkcam.mk

#-----------------------------------------------------------
LOCAL_CFLAGS += $(MTKCAM_CFLAGS)

#-----------------------------------------------------------
LOCAL_C_INCLUDES += $(MTKCAM_C_INCLUDES)

#-----------------------------------------------------------
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/include
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_PLATFORM)/hardware/include/D2

# End of common part ---------------------------------------
#
include $(BUILD_SHARED_LIBRARY)


#
#include $(call all-makefiles-under, $(LOCAL_PATH))
