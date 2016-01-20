LOCAL_PATH := $(call my-dir)  
include $(CLEAR_VARS)  
LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog -lEGL  
LOCAL_MODULE    := mystrcmp  
LOCAL_SRC_FILES := mystrcmp.c  
include $(BUILD_SHARED_LIBRARY)