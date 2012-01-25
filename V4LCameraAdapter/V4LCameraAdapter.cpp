/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/**
* @file V4LCameraAdapter.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/


#include "V4LCameraAdapter.h"
#include "CameraHal.h"
#include "TICameraParameters.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev.h>


#include <cutils/properties.h>
#define UNLIKELY( exp ) (__builtin_expect( (exp) != 0, false ))
static int mDebugFps = 0;

#define Q16_OFFSET 16

#define HERE(Msg) {CAMHAL_LOGEB("--===line %d, %s===--\n", __LINE__, Msg);}

namespace android {

#undef LOG_TAG
///Maintain a separate tag for V4LCameraAdapter logs to isolate issues OMX specific
#define LOG_TAG "V4LCameraAdapter"

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

static V4LCameraAdapter *gCameraAdapter = NULL;
Mutex gAdapterLock;
const char *device = DEVICE;


/*--------------------Camera Adapter Class STARTS here-----------------------------*/

status_t V4LCameraAdapter::initialize(int sensor_index)
{
    LOG_FUNCTION_NAME

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);

    int ret = NO_ERROR;

    // Allocate memory for video info structure
    mVideoInfo = (struct VideoInfo *) calloc (1, sizeof (struct VideoInfo));
    if(!mVideoInfo)
        {
        return NO_MEMORY;
        }

    if ((mCameraHandle = open(device, O_RDWR)) == -1)
        {
        CAMHAL_LOGEB("Error while opening handle to V4L2 Camera: %s", strerror(errno));
        return -EINVAL;
        }

    ret = ioctl (mCameraHandle, VIDIOC_QUERYCAP, &mVideoInfo->cap);
    if (ret < 0)
        {
        CAMHAL_LOGEA("Error when querying the capabilities of the V4L Camera");
        return -EINVAL;
        }

    if ((mVideoInfo->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0)
        {
        CAMHAL_LOGEA("Error while adapter initialization: video capture not supported.");
        return -EINVAL;
        }

    if (!(mVideoInfo->cap.capabilities & V4L2_CAP_STREAMING))
        {
        CAMHAL_LOGEA("Error while adapter initialization: Capture device does not support streaming i/o");
        return -EINVAL;
        }

    // Initialize flags
    mPreviewing = false;
    mVideoInfo->isStreaming = false;
    mRecording = false;

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t V4LCameraAdapter::fillThisBuffer(void* frameBuf, CameraFrame::FrameType frameType)
{

    status_t ret = NO_ERROR;

    if ( !mVideoInfo->isStreaming )
        {
        return NO_ERROR;
        }

    int i = mPreviewBufs.valueFor(( unsigned int )frameBuf);
    if(i<0)
        {
        return BAD_VALUE;
        }

    mVideoInfo->buf.index = i;
    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
    if (ret < 0) {
       CAMHAL_LOGEA("Init: VIDIOC_QBUF Failed");
       return -1;
    }

     nQueued++;

    return ret;

}

status_t V4LCameraAdapter::getCaps(CameraParameters &params)
{
    LOG_FUNCTION_NAME
    status_t ret = NO_ERROR;
    // For now do not populate anything
    //@todo Enhance this method to populate the capabilities queried from the V4L device.
    LOG_FUNCTION_NAME_EXIT
    return ret;
}

int V4LCameraAdapter::getRevision()
{
    LOG_FUNCTION_NAME

    LOG_FUNCTION_NAME_EXIT

    // For now return version 0
    //@todo Need to query and report the version of the V4L driver
    return 0;
}

status_t V4LCameraAdapter::setParameters(const CameraParameters &params)
{
    LOG_FUNCTION_NAME

    status_t ret = NO_ERROR;

    int width, height;

    params.getPreviewSize(&width, &height);

    CAMHAL_LOGDB("Width * Height %d x %d format 0x%x", width, height, DEFAULT_PIXEL_FORMAT);

    mVideoInfo->width = width;
    mVideoInfo->height = height;
    mVideoInfo->framesizeIn = (width * height << 1);
    mVideoInfo->formatIn = DEFAULT_PIXEL_FORMAT;

    mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->format.fmt.pix.width = width;
    mVideoInfo->format.fmt.pix.height = height;
    mVideoInfo->format.fmt.pix.pixelformat = DEFAULT_PIXEL_FORMAT;

    ret = ioctl(mCameraHandle, VIDIOC_S_FMT, &mVideoInfo->format);
    if (ret < 0) {
        CAMHAL_LOGEB("Open: VIDIOC_S_FMT Failed: %s", strerror(errno));
        return ret;
    }

    // Udpate the current parameter set
    mParams = params;

    LOG_FUNCTION_NAME_EXIT
    return ret;
}


void V4LCameraAdapter::getParameters(CameraParameters& params)
{
    LOG_FUNCTION_NAME

    // Return the current parameter set
    params = mParams;

    LOG_FUNCTION_NAME_EXIT
}


///API to give the buffers to Adapter
status_t V4LCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME

    Mutex::Autolock lock(mLock);

    switch(mode)
        {
        case CAMERA_PREVIEW:
            ret = UseBuffersPreview(bufArr, num);
            break;

        //@todo Insert Image capture case here

        case CAMERA_VIDEO:
            //@warn Video capture is not fully supported yet
            ret = UseBuffersPreview(bufArr, num);
            break;

        }

    LOG_FUNCTION_NAME_EXIT

    return ret;
}

status_t V4LCameraAdapter::UseBuffersPreview(void* bufArr, int num)
{
    int ret = NO_ERROR;

    if(NULL == bufArr)
        {
        return BAD_VALUE;
        }

    //First allocate adapter internal buffers at V4L level for USB Cam
    //These are the buffers from which we will copy the data into overlay buffers
    /* Check if camera can handle NB_BUFFER buffers */
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
    mVideoInfo->rb.count = num;

    ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    if (ret < 0) {
        CAMHAL_LOGEB("VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }

    for (int i = 0; i < num; i++) {

        memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));

        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            CAMHAL_LOGEB("Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        mVideoInfo->mem[i] = mmap (0,
               mVideoInfo->buf.length,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               mCameraHandle,
               mVideoInfo->buf.m.offset);

        if (mVideoInfo->mem[i] == MAP_FAILED) {
            CAMHAL_LOGEB("Unable to map buffer (%s)", strerror(errno));
            return -1;
        }

        uint32_t *ptr = (uint32_t*) bufArr;

        //Associate each Camera internal buffer with the one from Overlay
        mPreviewBufs.add((int)ptr[i], i);

    }

    // Update the preview buffer count
    mPreviewBufferCount = num;

    return ret;
}

status_t V4LCameraAdapter::startPreview()
{
    status_t ret = NO_ERROR;

  Mutex::Autolock lock(mPreviewBufsLock);

  if(mPreviewing)
    {
    return BAD_VALUE;
    }

   for (int i = 0; i < mPreviewBufferCount; i++) {

       mVideoInfo->buf.index = i;
       mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
       mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

       ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
       if (ret < 0) {
           CAMHAL_LOGEA("VIDIOC_QBUF Failed");
           return -EINVAL;
       }

       nQueued++;
   }

    enum v4l2_buf_type bufType;
   if (!mVideoInfo->isStreaming) {
       bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

       ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);
       if (ret < 0) {
           CAMHAL_LOGEB("StartStreaming: Unable to start capture: %s", strerror(errno));
           return ret;
       }

       mVideoInfo->isStreaming = true;
   }

   // Create and start preview thread for receiving buffers from V4L Camera
   mPreviewThread = new PreviewThread(this);

   CAMHAL_LOGDA("Created preview thread");


   //Update the flag to indicate we are previewing
   mPreviewing = true;

   return ret;

}

status_t V4LCameraAdapter::stopPreview()
{
    enum v4l2_buf_type bufType;
    int ret = NO_ERROR;

    Mutex::Autolock lock(mPreviewBufsLock);

    if(!mPreviewing)
        {
        return NO_INIT;
        }

    if (mVideoInfo->isStreaming) {
        bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
        if (ret < 0) {
            CAMHAL_LOGEB("StopStreaming: Unable to stop capture: %s", strerror(errno));
            return ret;
        }

        mVideoInfo->isStreaming = false;
    }

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    nQueued = 0;
    nDequeued = 0;

    /* Unmap buffers */
    for (int i = 0; i < mPreviewBufferCount; i++)
        if (munmap(mVideoInfo->mem[i], mVideoInfo->buf.length) < 0)
            CAMHAL_LOGEA("Unmap failed");

    mPreviewBufs.clear();

    mPreviewThread->requestExitAndWait();
    mPreviewThread.clear();

    return ret;

}

char * V4LCameraAdapter::GetFrame(int &index)
{
    int ret;

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

    /* DQ */
    ret = ioctl(mCameraHandle, VIDIOC_DQBUF, &mVideoInfo->buf);
    if (ret < 0) {
        CAMHAL_LOGEA("GetFrame: VIDIOC_DQBUF Failed");
        return NULL;
    }
    nDequeued++;

    index = mVideoInfo->buf.index;

    return (char *)mVideoInfo->mem[mVideoInfo->buf.index];
}

status_t V4LCameraAdapter::setTimeOut(unsigned int sec)
{
    status_t ret = NO_ERROR;

    gCameraAdapter = NULL;
    delete this;

    return ret;
}

//API to get the frame size required to be allocated. This size is used to override the size passed
//by camera service when VSTAB/VNF is turned ON for example
void V4LCameraAdapter::getFrameSize(int &width, int &height)
{
    // Just return the current preview size, nothing more to do here.
    mParams.getPreviewSize(&width, &height);

    LOG_FUNCTION_NAME_EXIT
}

int V4LCameraAdapter::getCameraCalStatus()
{
    return 0;
}

bool V4LCameraAdapter::getCameraModuleQueryString(char *str, unsigned long length)
{
    return true;
}

status_t V4LCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    // We don't support meta data, so simply return
    return NO_ERROR;
}

status_t V4LCameraAdapter::getPictureBufferSize(size_t &length, size_t bufferCount)
{
    // We don't support image capture yet, safely return from here without messing up
    return NO_ERROR;
}

static void debugShowFPS()
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if (!(mFrameCount & 0x1F)) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        LOGD("Camera %d Frames, %f FPS", mFrameCount, mFps);
    }
    // XXX: mFPS has the value we want
}

status_t V4LCameraAdapter::recalculateFPS()
{
    float currentFPS;

    mFrameCount++;

    if ( ( mFrameCount % FPS_PERIOD ) == 0 )
        {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter )
            {
            mFPS = currentFPS;
            }
        else
            {
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
            }

        mLastFPS = mFPS;
        mIter++;
        }

    return NO_ERROR;
}


V4LCameraAdapter::V4LCameraAdapter()
{
    LOG_FUNCTION_NAME

    // Nothing useful to do in the constructor

    LOG_FUNCTION_NAME_EXIT
}

V4LCameraAdapter::~V4LCameraAdapter()
{
    LOG_FUNCTION_NAME

    // Close the camera handle and free the video info structure
    close(mCameraHandle);

    if (mVideoInfo)
      {
        free(mVideoInfo);
        mVideoInfo = NULL;
      }
    LOG_FUNCTION_NAME_EXIT
}

/* Preview Thread */
// ---------------------------------------------------------------------------

int V4LCameraAdapter::previewThread()
{
    status_t ret = NO_ERROR;
    int width, height;
    CameraFrame frame;

    if (mPreviewing)
        {
        int index = 0;
        char *fp = this->GetFrame(index);
        if(!fp)
            {
            return BAD_VALUE;
            }

        uint8_t* ptr = (uint8_t*) mPreviewBufs.keyAt(index);

        int width, height;
        uint16_t* dest = (uint16_t*)ptr;
        uint16_t* src = (uint16_t*) fp;
        mParams.getPreviewSize(&width, &height);
        for(int i=0;i<height;i++)
            {
            for(int j=0;j<width;j++)
                {
                //*dest = *src;
                //convert from YUYV to UYVY supported in Camera service
                *dest = (((*src & 0xFF000000)>>24)<<16)|(((*src & 0x00FF0000)>>16)<<24) |
                        (((*src & 0xFF00)>>8)<<0)|(((*src & 0x00FF)>>0)<<8);
                src++;
                dest++;
                }
                dest += 4096/2-width;
            }

        mParams.getPreviewSize(&width, &height);
        frame.mFrameType = CameraFrame::PREVIEW_FRAME_SYNC;
        frame.mBuffer = ptr;
        frame.mLength = width*height*2;
        frame.mAlignment = width*2;
        frame.mOffset = 0;
        frame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);;

        resetFrameRefCount(frame);

        ret = sendFrameToSubscribers(&frame);

        }

    return ret;
}

extern "C" CameraAdapter* CameraAdapter_Factory()
{
    Mutex::Autolock lock(gAdapterLock);

    LOG_FUNCTION_NAME

    if ( NULL == gCameraAdapter )
        {
        CAMHAL_LOGDA("Creating new Camera adapter instance");
        gCameraAdapter= new V4LCameraAdapter();
        }
    else
        {
        CAMHAL_LOGDA("Reusing existing Camera adapter instance");
        }


    LOG_FUNCTION_NAME_EXIT

    return gCameraAdapter;
}

};


/*--------------------Camera Adapter Class ENDS here-----------------------------*/

