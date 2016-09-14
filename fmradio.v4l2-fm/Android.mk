LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libfmradio.v4l2-fm
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAG := optional
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SRC_FILES := v4l2_fm.c v4l2_ioctl.c v4l2_ioctl.h
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := radio_v4l
LOCAL_SRC_FILES := radio_v4l.c
LOCAL_MODULE_TAG := optional
LOCAL_SHARED_LIBRARIES := libtinyalsa libfmradio.v4l2-fm
LOCAL_C_INCLUDES := \
	external/tinyalsa/include \
	hardware/libhardware/include
include $(BUILD_EXECUTABLE)
