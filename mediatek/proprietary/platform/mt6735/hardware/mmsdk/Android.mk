LOCAL_PATH:= $(call my-dir)

ifneq ($(strip $(TARGET_BOARD_PLATFORM)),mt6735m)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
