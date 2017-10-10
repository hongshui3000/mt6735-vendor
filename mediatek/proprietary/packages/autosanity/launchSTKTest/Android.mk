#ifeq ($(strip $(MTK_AUTO_TEST)),yes)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# We only want this apk build for tests.
LOCAL_MODULE_TAGS := tests

LOCAL_JAVA_LIBRARIES := android.test.runner

LOCAL_CERTIFICATE := platform

# Include all test java files.

LOCAL_STATIC_JAVA_LIBRARIES := autoTest_lib libjunitreport-for-stk-tests librobotium4
                              
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := launchstktest

LOCAL_SDK_VERSION := current

#LOCAL_INSTRUMENTATION_FOR := launchstktest


include $(BUILD_PACKAGE)
##################################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := libjunitreport-for-stk-tests:libs/android-junit-report-dev.jar autoTest_lib:autoTest_Telephony.jar

include $(BUILD_MULTI_PREBUILT)

#endif
