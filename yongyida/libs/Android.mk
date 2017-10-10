LOCAL_PATH:= $(call my-dir)
#ifeq ($(MTK_TARGET_PROJECT),$(MTK_BASE_PROJECT))
#include $(call all-makefiles-under,$(LOCAL_PATH)/$(TARGET_DEVICE))
#else
#include $(call all-makefiles-under,$(LOCAL_PATH)/$(MTK_TARGET_PROJECT))
#endif

product = $(word 1, $(subst _, $(space), $(MTK_PROJECT)))
ifeq ($(product), y50b)
    include $(call all-makefiles-under,$(LOCAL_PATH)/common)
endif