LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := zygisk
LOCAL_SRC_FILES := ../zygisk/main.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../zygisk
include $(BUILD_SHARED_LIBRARY)
