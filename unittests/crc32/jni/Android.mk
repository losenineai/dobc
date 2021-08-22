LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE :=  crc32
LOCAL_SRC_FILES := crc32.c
LOCAL_LDLIBS := -L${SYSROOT}/usr/lib -llog -fPIE

include $(BUILD_SHARED_LIBRARY)
