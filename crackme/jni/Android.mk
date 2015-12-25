LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := crackme
LOCAL_SRC_FILES := crackme.c
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)
