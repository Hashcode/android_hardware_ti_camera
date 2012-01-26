ifeq ($(BOARD_USES_TI_CAMERA_HAL),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#######

#
# LIBCAMERA
#

LIBCAMERA_INTERMEDIATES_PREREQS := $(PRODUCT_OUT)/obj/lib

# prerequisites for building libcamera
file := $(LIBCAMERA_INTERMEDIATES_PREREQS)/libhdr_interface.so
$(file) : $(TOP)/hardware/ti/camera/prebuilt/libhdr_interface.so
	@echo "Copy libhdr_interface.so -> $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) cp -a $(TOP)/hardware/ti/camera/prebuilt/libhdr_interface.so $@

LOCAL_SRC_FILES:= \
    CameraHalM.cpp \
    CameraHal.cpp \
    CameraHalUtilClasses.cpp \
    AppCallbackNotifier.cpp \
    MemoryManager.cpp	\
    ANativeWindowDisplayAdapter.cpp \
    CameraProperties.cpp \
    TICameraParameters.cpp \


LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/inc/HDRInterface \
    $(LOCAL_PATH)/../omap4xxx/domx/system/mm_osal/inc \
    $(LOCAL_PATH)/../omap4xxx/domx/system/omx_core/inc \
    $(LOCAL_PATH)/../omap4xxx/hwc \
    $(LOCAL_PATH)/../omap4xxx/libtiutils \
    $(LOCAL_PATH)/../tiler \
    $(LOCAL_PATH)/../syslink/ipc-listener \
    $(TOP)/external/libxml2/include \
    $(TOP)/external/icu4c/common \
    $(TOP)/frameworks/base/include/utils \
    $(TOP)/frameworks/base/services/camera/libcameraservice \
    $(TOP)/frameworks/base/include/media/stagefright \
    $(TOP)/frameworks/base/include/media/stagefright/openmax \
    $(TOP)/system/core/include/system \

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libtimemmgr \
    libicuuc \
    libcamera_client \
    libhdr_interface \
    libsyslink_ipc_listener \

LOCAL_STATIC_LIBRARIES:= \
    libxml2

LOCAL_CFLAGS += -fno-short-enums -DCOPY_IMAGE_BUFFER -DTARGET_OMAP4 -mfpu=neon

LOCAL_MODULE:= libcamera
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)


#######

#
# OMX Camera Adapter 
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    BaseCameraAdapter.cpp \
    OMXCameraAdapter/OMXCap.cpp \
    OMXCameraAdapter/OMXCameraAdapter.cpp \

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/ \
    $(LOCAL_PATH)/inc/HDRInterface \
    $(LOCAL_PATH)/inc/OMXCameraAdapter \
    $(LOCAL_PATH)/../omap4xxx/hwc \
    $(LOCAL_PATH)/../omap4xxx/libtiutils \
    $(LOCAL_PATH)/../omap4xxx/domx/system/omx_core/inc \
    $(LOCAL_PATH)/../omap4xxx/domx/system/mm_osal/inc \
    $(TOP)/external/libxml2/include \
    $(TOP)/external/icu4c/common \
    $(TOP)/bionic/libc/include \
    $(TOP)/frameworks/base/include/ui \
    $(TOP)/frameworks/base/include/utils \
    $(TOP)/frameworks/base/services/camera/libcameraservice \
    $(TOP)/system/core/include/system \

LOCAL_SHARED_LIBRARIES:= \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libOMX_CoreOsal \
    libOMX_Core \
    libsysmgr \
    librcm \
    libipc \
    libcamera \
    libicuuc \
    libcamera_client \
    libomx_rpc \
    libhdr_interface \
    libhardware_legacy \

LOCAL_CFLAGS += -fno-short-enums -DTARGET_OMAP4

LOCAL_MODULE:= libomxcameraadapter
LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)

#######

#
# OMX Camera HAL 
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    CameraHal_Module.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/inc/HDRInterface \
    $(LOCAL_PATH)/inc/OMXCameraAdapter \
    $(LOCAL_PATH)/../omap4xxx/libtiutils \
    $(LOCAL_PATH)/../omap4xxx/hwc \
    $(LOCAL_PATH)/../omap4xxx/domx/system/omx_core/inc \
    $(LOCAL_PATH)/../omap4xxx/domx/system/mm_osal/inc \
    $(LOCAL_PATH)/../tiler \
    $(LOCAL_PATH)/../syslink/ipc-listener \
    $(TOP)/bionic/libc/include \
    $(TOP)/frameworks/base/include/ui \
    $(TOP)/frameworks/base/include/utils \
    $(TOP)/frameworks/base/services/camera/libcameraservice \
    $(TOP)/external/libxml2/include \
    $(TOP)/external/icu4c/common \
    $(TOP)/system/core/include/system \


LOCAL_SHARED_LIBRARIES := \
    libcamera \
    libomxcameraadapter \
    libdl \
    libui \
    libbinder \
    libutils \
    libcutils \
    libtiutils \
    libOMX_CoreOsal \
    libOMX_Core \
    libsysmgr \
    librcm \
    libipc \
    libcamera \
    libicuuc \
    libcamera_client \
    libomx_rpc \
    libhdr_interface \
    libhardware_legacy \

LOCAL_STATIC_LIBRARIES := \
    libxml2 \

LOCAL_CFLAGS += -fno-short-enums -DCOPY_IMAGE_BUFFER -DTARGET_OMAP4 -mfpu=neon

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE := camera.omap4
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
