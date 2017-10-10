LOCAL_PATH:= $(call my-dir)
ifeq ($(MTK_PROJECT), y50b_dev_new)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
