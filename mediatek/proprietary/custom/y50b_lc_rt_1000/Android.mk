LOCAL_PATH:= $(call my-dir)
ifeq ($(MTK_PROJECT), y50b_lc_rt_1000)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
