
// This is the CameraHalImpl class, which inherits from CameraHal and CameraTCInterface.h

#ifndef __CAMERA_HAL_VIRTUAL_H__
#define __CAMERA_HAL_VIRTUAL_H__

#include "CameraHardwareInterface.h"
#include "CameraTCInterface.h"
// #include <CameraSettings.h>
#include <linux/i2c.h>

#define MAX_CAP_MODE_SIZE 32

/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data {
	struct i2c_msg __user *msgs;	/* pointers to i2c_msgs */
	__u32 nmsgs;			/* number of i2c_msgs */
};

namespace android {

class CameraHalMotBase : public CameraHardwareInterface, public CameraTCInterface {
protected:
    bool mBurstEnabled;
    char mLastCapMode[MAX_CAP_MODE_SIZE];
    bool bCameraTestEnabled;
    unsigned long arrDebugAttribs[DebugAttrib_NUM];
    bool mSdmDisabled;

    class WatchDogThread : public Thread
    {
        Mutex mSuccessLock;
        Condition mSuccessCond;
        bool success;
        public:
        WatchDogThread() : Thread(false), success(false) {}
        virtual bool threadLoop();
        virtual void onFirstRef()
        {
            run("CameraWatchDogThread", PRIORITY_NORMAL);
        }
        void signalSuccess()
        {
            Mutex::Autolock lock(mSuccessLock);
            success = true;
            mSuccessCond.signal();
        }
    };

    class AutoWatchDog {
    public:
        inline AutoWatchDog() {startWatchDog();}
        inline ~AutoWatchDog() {stopWatchDog();}
    private:
        void startWatchDog();
        void stopWatchDog();
        sp<WatchDogThread> mWatchDog;
    };

};



} // namespace android






#endif // __CAMERA_HAL_VIRTUAL_H__
