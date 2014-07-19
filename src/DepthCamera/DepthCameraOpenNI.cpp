// DepthCameraOpenNI.cpp

#include "DepthCameraOpenNI.h"

#if USE_OPENNI

/// Helper function
///@todo Use Logger class
void printXnError(XnStatus status)
{
    if (status == XN_STATUS_OK)
        return;

    XnUInt16 group = XN_STATUS_GROUP(status);
    XnUInt16 code  = XN_STATUS_CODE(status);

    const char* pErrorStr = xnGetStatusString(status);
    if (pErrorStr != NULL)
    {
        printf("Xn ERROR: group %d, code %d: %s\n", group, code, pErrorStr);
        //LOG_INFO("Xn ERROR: group %d, code %d: %s\n", group, code, pErrorStr);
    }
}


DepthCameraOpenNI::DepthCameraOpenNI()
: DepthCamera()
, m_status(XN_STATUS_OK)
, m_Context()
, m_DepthGenerator()
, m_ImageGenerator()
, m_depthMD()
, m_imageMD()
, m_sceneMD()
{
    ///@note Attempting to re-create this every call to UpdateFrame results in a memory leak.
    m_status = m_Context.Init();
    printXnError(m_status);

    if (m_status == XN_STATUS_OK)
    {
        m_status = _Initialize();
    }
    
    if (m_status != XN_STATUS_OK)
    {
        printf("OpenNIDepthCamera ctor failed: %s\n", xnGetStatusString(m_status));
    }
}

DepthCameraOpenNI::~DepthCameraOpenNI()
{
    _Uninitialize();
    m_Context.Release();
}

int DepthCameraOpenNI::_Initialize()
{
    m_depthWidth = 640; m_depthHeight = 480;
    m_colorWidth = 640; m_colorHeight = 480;

    /// Init depth stream
    m_status = m_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, m_DepthGenerator);
    /// No match here is generally expected.
    //printXnError(m_status);

    if (m_status != XN_STATUS_OK)
    {
        xn::DepthGenerator dg;
        m_status = dg.Create(m_Context);
        ///@todo Print the error message only once.
        printXnError(m_status);

        if (m_status == XN_STATUS_OK)
        {
            // set some defaults
            XnMapOutputMode defaultMode;
            defaultMode.nXRes = m_depthWidth;
            defaultMode.nYRes = m_depthHeight;
            defaultMode.nFPS = m_fps;
            m_status = dg.SetMapOutputMode(defaultMode);

            m_DepthGenerator = dg;
        }
    }

    /// Already failed, not much more we can do so return.
    if (m_status != XN_STATUS_OK)
    {
        return m_status;
    }

    /// Init color stream
    m_status = m_Context.FindExistingNode(XN_NODE_TYPE_IMAGE, m_ImageGenerator);
    /// No match here is generally expected.
    //printXnError(m_status);

    if (m_status != XN_STATUS_OK)
    {
        xn::ImageGenerator ig;
        m_status = ig.Create(m_Context);
        printXnError(m_status);

        if (m_status == XN_STATUS_OK)
        {
            // set some defaults
            XnMapOutputMode defaultMode;
            defaultMode.nXRes = m_colorWidth;
            defaultMode.nYRes = m_colorHeight;
            defaultMode.nFPS = m_fps;
            m_status = ig.SetMapOutputMode(defaultMode);

            m_ImageGenerator = ig;
        }
    }

    if (m_status == XN_STATUS_OK)
    {
        m_status = m_Context.StartGeneratingAll();
        printXnError(m_status);
    }
    
    if (m_status == XN_STATUS_OK)
    {
        _AllocateDepthAndColorBuffers();
    }

    return m_status;
}

int DepthCameraOpenNI::_Uninitialize()
{
    m_DepthGenerator.Release();
    m_ImageGenerator.Release();

    return 0;
}

///@brief Copy depth data into local buffers
void DepthCameraOpenNI::_CopyDepthImage()
{
    if (m_depthBuffer.empty())
        return;

    const unsigned int dim = m_depthWidth * m_depthHeight;

    m_DepthGenerator.GetMetaData(m_depthMD);
    const XnDepthPixel* depthRaw = m_depthMD.Data();
    if (depthRaw == NULL)
        return;

    memcpy(&m_depthBuffer[0], depthRaw, dim * sizeof(unsigned short));
}

///@brief Copy depth data into local buffers
void DepthCameraOpenNI::_CopyColorImage()
{
    if (m_colorBuffer.empty())
        return;

    m_ImageGenerator.GetMetaData(m_imageMD);
    const XnRGB24Pixel* pColor = m_imageMD.RGB24Data();
    if (pColor == NULL)
        return;

    const unsigned int dim = m_depthWidth * m_depthHeight;
    ///@note We use RGB here, not RGBA
    memcpy(&m_colorBuffer[0], pColor, dim * 3*sizeof(unsigned char));
}

void DepthCameraOpenNI::UpdateFrame()
{
    m_status = m_Context.WaitAnyUpdateAll();
    printXnError(m_status);

    _CopyDepthImage();
    _CopyColorImage();
}

#endif //USE_OPENNI
