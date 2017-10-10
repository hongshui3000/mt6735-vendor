LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := bootaudio.mp3
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := media

product := $(word 1, $(subst _, $(space), $(MTK_PROJECT)))
ifeq ($(product), y50b)
	ifeq ($(MTK_PROJECT), y50b_dev_new)
	    src := y50b_dev/bootaudio.mp3
	else
	    src := $(MTK_PROJECT)/bootaudio.mp3
	endif
endif

LOCAL_SRC_FILES := $(src)

LOCAL_MODULE_PATH := $(TARGET_OUT)/media

include $(BUILD_PREBUILT)


