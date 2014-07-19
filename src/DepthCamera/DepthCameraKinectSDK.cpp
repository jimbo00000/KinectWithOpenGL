// DepthCameraKinectSDK.cpp

#include "DepthCameraKinectSDK.h"
#include <stdio.h>

#if USE_KINECTSDK

DepthCameraKinectSDK::DepthCameraKinectSDK()
: DepthCamera()
, m_bSeatedMode(false)
, m_pNuiSensor(NULL)
, m_pDepthStreamHandle(INVALID_HANDLE_VALUE)
, m_hNextDepthFrameEvent(INVALID_HANDLE_VALUE)
, m_pColorStreamHandle(INVALID_HANDLE_VALUE)
, m_hNextColorFrameEvent(INVALID_HANDLE_VALUE)
, m_pSkeletonStreamHandle(INVALID_HANDLE_VALUE)
, m_hNextSkeletonEvent(INVALID_HANDLE_VALUE)
, m_bNearMode(false)
, m_depthRes(NUI_IMAGE_RESOLUTION_640x480)
, m_colorRes(NUI_IMAGE_RESOLUTION_640x480)
//NUI_IMAGE_RESOLUTION_80x60
//NUI_IMAGE_RESOLUTION_320x240
//NUI_IMAGE_RESOLUTION_640x480
//NUI_IMAGE_RESOLUTION_1280x960
{
    _Initialize();
}

DepthCameraKinectSDK::~DepthCameraKinectSDK()
{
    _Uninitialize();
}

void SetWidthAndHeightByConstant(NUI_IMAGE_RESOLUTION res, unsigned int& w, unsigned int& h)
{
    switch(res)
    {
    default:
    case NUI_IMAGE_RESOLUTION_640x480:
        w = 640;
        h = 480;
        break;
    case NUI_IMAGE_RESOLUTION_80x60:
        w = 80;
        h = 60;
        break;
    case NUI_IMAGE_RESOLUTION_320x240:
        w = 320;
        h = 240;
        break;
    case NUI_IMAGE_RESOLUTION_1280x960:
        w = 1280;
        h = 960;
        break;
    }
}

int DepthCameraKinectSDK::_Initialize()
{
    INuiSensor * pNuiSensor;
    HRESULT hr;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using depth
        hr = m_pNuiSensor->NuiInitialize(
            NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX |
            NUI_INITIALIZE_FLAG_USES_COLOR
            );

        if (SUCCEEDED(hr))
        {
            // Create an event that will be signaled when depth data is available
            m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            // Open a depth image stream to receive depth frames
            hr = m_pNuiSensor->NuiImageStreamOpen(
                NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,
                m_depthRes,
                0,
                2,
                m_hNextDepthFrameEvent,
                &m_pDepthStreamHandle);
            
            // Create an event that will be signaled when color data is available
            m_hNextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            // Open a color image stream to receive color frames
            hr = m_pNuiSensor->NuiImageStreamOpen(
                NUI_IMAGE_TYPE_COLOR,
                m_colorRes,
                0,
                2,
                m_hNextColorFrameEvent,
                &m_pColorStreamHandle);

            // Create an event that will be signaled when skeleton data is available
            m_hNextSkeletonEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

            // Open a skeleton stream to receive skeleton data
            hr = m_pNuiSensor->NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0);
        }
    }

    if (NULL == m_pNuiSensor || FAILED(hr))
    {
        //SetStatusMessage(L"No ready Kinect found!");
        return E_FAIL;
    }

    /// Set resolution fields based on constant
    SetWidthAndHeightByConstant(m_depthRes, m_depthWidth, m_depthHeight);
    SetWidthAndHeightByConstant(m_colorRes, m_colorWidth, m_colorHeight);

    //m_currentStatus = OK;
    _AllocateDepthAndColorBuffers();

    return hr;
}

void CALLBACK StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void* pUserData)
{
    // http://msdn.microsoft.com/en-us/library/nuiapi.nuisetdevicestatuscallback.aspx
    if ( SUCCEEDED( hrStatus ) )
    {
        // Initialize the Kinect sensor identified by the instanceName parameter.      
        printf("StatusProc : success\n");
    }
    else
    {
        // Uninitialize the Kinect sensor identified by the instanceName parameter.        
        printf("StatusProc : FAILED\n");
    }
}

int DepthCameraKinectSDK::_Uninitialize()
{
    ///@todo According to the link above, this was supposed to allow us to Shutdown without hanging.
    /// Now at least the app gets one change on a new instance from live to recorded.
    NuiSetDeviceStatusCallback( &StatusProc, this );

    if (m_pNuiSensor)
    {
        ///m_pNuiSensor->NuiShutdown();
    }

    if (m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextDepthFrameEvent);
    }

    if (m_hNextColorFrameEvent && (m_hNextColorFrameEvent != INVALID_HANDLE_VALUE))
    {
        CloseHandle(m_hNextColorFrameEvent);
    }

    if (m_hNextSkeletonEvent && (m_hNextSkeletonEvent != INVALID_HANDLE_VALUE))
    {
        CloseHandle(m_hNextSkeletonEvent);
    }

    return 0;
}

/// Copy depth pixels from locked KinectSDK buffer into our local depth buffer.
void DepthCameraKinectSDK::_ProcessDepth()
{
    HRESULT hr;
    NUI_IMAGE_FRAME imageFrame;

    // Attempt to get the depth frame
    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);
    if (FAILED(hr))
    {
        return;
    }

    BOOL nearMode;
    INuiFrameTexture* pTexture;

    // Get the depth image pixel texture
    hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(
        m_pDepthStreamHandle, &imageFrame, &nearMode, &pTexture);
    if (FAILED(hr))
    {
        goto ReleaseFrame;
    }

    NUI_LOCKED_RECT LockedRect;

    // Lock the frame data so the Kinect knows not to modify it while we're reading it
    pTexture->LockRect(0, &LockedRect, NULL, 0);

    // Make sure we've received valid data
    if (LockedRect.Pitch != 0)
    {
        // Get the min and max reliable depth for the current frame
        //int minDepth = (nearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
        //int maxDepth = (nearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
        const NUI_DEPTH_IMAGE_PIXEL * pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits);

        // end pixel is start + width*height - 1
        const NUI_DEPTH_IMAGE_PIXEL * pBufferEnd = pBufferRun + (m_depthWidth * m_depthHeight);

        int i = 0;
        while (pBufferRun < pBufferEnd)
        {
            // discard the portion of the depth that contains only the player index
            USHORT depth = pBufferRun->depth;
            m_depthBuffer[i++] = depth;

            // Increment our index into the Kinect's depth buffer
            ++pBufferRun;
        }
    }

    // We're done with the texture so unlock it
    pTexture->UnlockRect(0);

    pTexture->Release();

ReleaseFrame:
    // Release the frame
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);
}

/// Copy the color image from the Kinect stream into the color buffer.
void DepthCameraKinectSDK::_ProcessColor()
{
    HRESULT hr;
    NUI_IMAGE_FRAME imageFrame;

    // Attempt to get the color frame
    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pColorStreamHandle, 0, &imageFrame);
    if (FAILED(hr))
    {
        return;
    }

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;

    // Lock the frame data so the Kinect knows not to modify it while we're reading it
    pTexture->LockRect(0, &LockedRect, NULL, 0);

    // Make sure we've received valid data
    if (LockedRect.Pitch != 0)
    {
        // Copy image data - check size
        const INT sz = m_colorWidth * m_colorHeight * 4; // RGBA
        if (sz != LockedRect.size)
        {
            //LOG_INFO("Color dimension mismatch: %d %d", m_colorWidth, m_colorHeight);
        }
        memcpy(&m_colorBuffer[0], static_cast<BYTE *>(LockedRect.pBits), LockedRect.size);
    }

    // We're done with the texture so unlock it
    pTexture->UnlockRect(0);

    // Release the frame
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_pColorStreamHandle, &imageFrame);
}

void DepthCameraKinectSDK::UpdateFrame()
{
    ///@todo Check if not initialized and attempt to do so again for hot-plug
    if (NULL == m_pNuiSensor)
        return;


    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextDepthFrameEvent, 0) )
    {
        _ProcessDepth();
        _ProcessColor();
        //_ScaleDepthMinMax();
    }
}

void DepthCameraKinectSDK::_ScaleDepthMinMax()
{
    unsigned short minDepth = USHRT_MAX;
    unsigned short maxDepth = 0;
    
    for (std::vector<unsigned short>::iterator it = m_depthBuffer.begin();
        it != m_depthBuffer.end();
        ++it)
    {
        if (*it == 0)
            continue;
        minDepth = std::min(*it, minDepth);
        maxDepth = std::max(*it, maxDepth);
    }
    if (minDepth == maxDepth)
        return;

    const unsigned short depthRange = maxDepth - minDepth;
    for (std::vector<unsigned short>::iterator it = m_depthBuffer.begin();
        it != m_depthBuffer.end();
        ++it)
    {
        unsigned short val = *it;
        if (val == 0)
            continue;
        const float fval = (
            static_cast<float>(val - minDepth) /
            static_cast<float>(maxDepth - minDepth)
            );
        const unsigned short normval = static_cast<unsigned short>(
            static_cast<float>(USHRT_MAX) * fval
            );
        *it = normval;
    }
}

#endif // USE_KINECTSDK
