// DepthCameraOpenNI.h

#pragma once

#include "DepthCamera.h"

#if USE_OPENNI
#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnPropNames.h>
#include <map>


class DepthCameraOpenNI : public DepthCamera
{
public:
    DepthCameraOpenNI();
    virtual ~DepthCameraOpenNI();
    virtual void UpdateFrame();

protected:
    virtual int _Initialize();
    virtual int _Uninitialize();

    virtual void _CopyDepthImage();
    virtual void _CopyColorImage();

    XnStatus           m_status;         ///< Camera status
    xn::Context        m_Context;        ///< OpenNI Context
    xn::DepthGenerator m_DepthGenerator; ///< OpenNI Depth Generator(depth camera)
    xn::ImageGenerator m_ImageGenerator; ///< OpenNI Image Generator(depth camera)
    xn::DepthMetaData  m_depthMD;        ///< depth metadata
    xn::ImageMetaData  m_imageMD;        ///< color metadata
    xn::SceneMetaData  m_sceneMD;        ///< scene metadata

private: // Disallow copy ctor and assignment operator
    DepthCameraOpenNI(const DepthCameraOpenNI&);
    DepthCameraOpenNI& operator=(const DepthCameraOpenNI&);
};

#endif //USE_OPENNI
