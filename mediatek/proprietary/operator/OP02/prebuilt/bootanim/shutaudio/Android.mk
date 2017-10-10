LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
product := $(word 1, $(subst _, $(space), $(MTK_PROJECT)))
ifeq ($(product), y50b)
	ifeq ($(strip $(MTK_PROJECT)), y50b_dev_new)
		src := y50b_dev/shutaudio.mp3
	else
	    src := $(MTK_PROJECT)/shutaudio.mp3
	endif
else 
	src := common/shutaudio.mp3
endif
LOCAL_MODULE := shutaudio.mp3
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := media
LOCAL_SRC_FILES := $(src)
LOCAL_MODULE_PATH := $(TARGET_OUT)/media
include $(BUILD_PREBUILT)

##########################
#
# 如果需要随机播放的第二个音频文件，shutaudio1.mp3
# 在device/yongyida/$(MTK_PROJECT)/device.mk 中
# 添加PRODUCT_COPY_FILES += xxxxx:xxxx
#
##########################
