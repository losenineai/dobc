LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE :=  md5
LOCAL_SRC_FILES := md5.c
LOCAL_LDLIBS := -L${SYSROOT}/usr/lib -llog -fPIE 

include $(BUILD_SHARED_LIBRARY)
