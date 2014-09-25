// DepthCameraKinectSDK2.h

#pragma once

#include "DepthCamera.h"

#if USE_KINECTSDK2

#ifdef WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

//#pragma warning ( disable : 4067 )
//#pragma warning ( disable : 4083 )
#include <Kinect.h>

class DepthCameraKinectSDK2 : public DepthCamera
{
public:
    DepthCameraKinectSDK2();
    virtual ~DepthCameraKinectSDK2();
    virtual void UpdateFrame();

protected:
    virtual int _Initialize();
    virtual int _Uninitialize();
    virtual void _ProcessDepth(UINT16* pBuffer);
    virtual void _ProcessColor();

    virtual void _ScaleDepthMinMax();


    // From DepthBasics example in Kinect SDK 2 Public Preview 1409
    IKinectSensor*     m_pKinectSensor;
    IDepthFrameReader* m_pDepthFrameReader;
    static const int   cDepthWidth  = 512;
    static const int   cDepthHeight = 424;

private: // Disallow copy ctor and assignment operator
    DepthCameraKinectSDK2(const DepthCameraKinectSDK2&);
    DepthCameraKinectSDK2& operator=(const DepthCameraKinectSDK2&);
};

#endif // USE_KINECTSDK2
