// DepthCameraKinectSDK.h

#pragma once

#include "DepthCamera.h"

#if USE_KINECTSDK

#ifdef WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif

#pragma warning ( disable : 4067 )
#pragma warning ( disable : 4083 )
#include "NuiApi.h"


class DepthCameraKinectSDK : public DepthCamera
{
public:
    DepthCameraKinectSDK();
    virtual ~DepthCameraKinectSDK();
    virtual void UpdateFrame();

protected:
    virtual int _Initialize();
    virtual int _Uninitialize();
    virtual void _ProcessDepth();
    virtual void _ProcessColor();

    virtual void _ScaleDepthMinMax();

    bool          m_bSeatedMode;
    INuiSensor*   m_pNuiSensor;
    HANDLE        m_pDepthStreamHandle;
    HANDLE        m_hNextDepthFrameEvent;
    HANDLE        m_pColorStreamHandle;
    HANDLE        m_hNextColorFrameEvent;
    HANDLE        m_pSkeletonStreamHandle;
    HANDLE        m_hNextSkeletonEvent;
    bool          m_bNearMode;

    NUI_IMAGE_RESOLUTION m_depthRes;
    NUI_IMAGE_RESOLUTION m_colorRes;

private: // Disallow copy ctor and assignment operator
    DepthCameraKinectSDK(const DepthCameraKinectSDK&);
    DepthCameraKinectSDK& operator=(const DepthCameraKinectSDK&);
};

#endif // USE_KINECTSDK
