ifeq ($(TARGET_BOARD_PLATFORM),omap4)

LOCAL_PATH:= $(call my-dir)

OMAP4_CAMERA_HAL_USES:= OMX

OMAP4_CAMERA_HAL_SRC := \
	CameraHal_Module.cpp \
	CameraHal.cpp \
	CameraHalUtilClasses.cpp \
	AppCallbackNotifier.cpp \
	ANativeWindowDisplayAdapter.cpp \
	CameraProperties.cpp \
	MemoryManager.cpp \
	Encoder_libjpeg.cpp \
	SensorListener.cpp  \
	NV12_resize.c \

	#v4l2_utils.c \

OMAP4_CAMERA_COMMON_SRC:= \
	CameraParameters.cpp \
	CameraParser.cpp \
	TICameraParameters.cpp \
	CameraHalCommon.cpp

OMAP4_CAMERA_OMX_SRC:= \
	BaseCameraAdapter.cpp \
	OMXCameraAdapter/OMX3A.cpp \
	OMXCameraAdapter/OMXAlgo.cpp \
	OMXCameraAdapter/OMXCameraAdapter.cpp \
	OMXCameraAdapter/OMXCapabilities.cpp \
	OMXCameraAdapter/OMXCapture.cpp \
	OMXCameraAdapter/OMXDefaults.cpp \
	OMXCameraAdapter/OMXExif.cpp \
	OMXCameraAdapter/OMXFD.cpp \
	OMXCameraAdapter/OMXFocus.cpp \
	OMXCameraAdapter/OMXZoom.cpp \

OMAP4_CAMERA_USB_SRC:= \
	BaseCameraAdapter.cpp \
	V4LCameraAdapter/V4LCameraAdapter.cpp

#
# OMX Camera HAL 
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	$(OMAP4_CAMERA_HAL_SRC) \
	$(OMAP4_CAMERA_OMX_SRC) \
	$(OMAP4_CAMERA_COMMON_SRC)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/ \
    $(LOCAL_PATH)/inc/OMXCameraAdapter \
    $(LOCAL_PATH)/../omap4xxx/hwc \
    $(LOCAL_PATH)/../omap4xxx/include \
    $(LOCAL_PATH)/../omap4xxx/libtiutils \
    $(LOCAL_PATH)/../tiler \
    $(TOP)/frameworks/base/include/ui \
    $(TOP)/frameworks/base/include/utils \
    $(LOCAL_PATH)/../omap4xxx/domx/system/omx_core/inc \
    $(LOCAL_PATH)/../omap4xxx/domx/system/mm_osal/inc \
    $(TOP)/frameworks/base/include/media/stagefright \
    $(TOP)/frameworks/base/include/media/stagefright/openmax \
    $(TOP)/external/jpeg \
    $(TOP)/external/jhead \
    $(TOP)/external/libxml2/include \
    $(TOP)/external/icu4c/common \

#    libmm_osal \
#    libdomx \
#    libion \

LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libtimemmgr \
    libOMX_Core \
    libOMX_CoreOsal \
    libcamera_client \
    libgui \
    libjpeg \
    libexif \
    libicuuc \

LOCAL_STATIC_LIBRARIES:= \
    libxml2

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.omap4
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)

endif
