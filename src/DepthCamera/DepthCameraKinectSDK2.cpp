// DepthCameraKinectSDK2.cpp

#include "DepthCameraKinectSDK2.h"
#include <stdio.h>
#include <Unknwn.h>

#if USE_KINECTSDK2

DepthCameraKinectSDK2::DepthCameraKinectSDK2()
: DepthCamera()
, m_pKinectSensor(NULL)
, m_pDepthFrameReader(NULL)
{
    _Initialize();
}

DepthCameraKinectSDK2::~DepthCameraKinectSDK2()
{
    _Uninitialize();
}

int DepthCameraKinectSDK2::_Initialize()
{
    HRESULT hr;

    hr = GetDefaultKinectSensor(&m_pKinectSensor);
    if (FAILED(hr))
    {
        return hr;
    }

    if (m_pKinectSensor)
    {
        // Initialize the Kinect and get the depth reader
        IDepthFrameSource* pDepthFrameSource = NULL;

        hr = m_pKinectSensor->Open();

        if (SUCCEEDED(hr))
        {
            hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
        }

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
        }

        //SafeRelease(pDepthFrameSource);
    }

    //m_currentStatus = OK;

    m_depthWidth = cDepthWidth;
    m_depthHeight = cDepthHeight;

    _AllocateDepthAndColorBuffers();

    return hr;
}


int DepthCameraKinectSDK2::_Uninitialize()
{
#if 0
    // done with depth frame reader
    if (m_pDepthFrameReader)
        m_pDepthFrameReader->Release;

    // close the Kinect Sensor
    if (m_pKinectSensor)
    {
        m_pKinectSensor->Close();
    }

    if (m_pKinectSensor)
        m_pKinectSensor->Release();
#endif
    return 0;
}

/// Copy depth pixels from locked KinectSDK buffer into our local depth buffer.
void DepthCameraKinectSDK2::_ProcessDepth(UINT16* pBuffer)
{
    memcpy(&m_depthBuffer[0], pBuffer, m_depthWidth * m_depthHeight * sizeof(UINT16));

#if 0
    // RGBA8

    // Make sure we've received valid data
    //if (m_pDepthRGBX && pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
    {
        RGBQUAD* pRGBX = m_pDepthRGBX;

        // end pixel is start + width*height - 1
        const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);

        while (pBuffer < pBufferEnd)
        {
            USHORT depth = *pBuffer;

            // To convert to a byte, we're discarding the most-significant
            // rather than least-significant bits.
            // We're preserving detail, although the intensity will "wrap."
            // Values outside the reliable depth range are mapped to 0 (black).

            // Note: Using conditionals in this loop could degrade performance.
            // Consider using a lookup table instead when writing production code.
            BYTE intensity = static_cast<BYTE>((depth >= nMinDepth) && (depth <= nMaxDepth) ? (depth % 256) : 0);

            pRGBX->rgbRed   = intensity;
            pRGBX->rgbGreen = intensity;
            pRGBX->rgbBlue  = intensity;

            ++pRGBX;
            ++pBuffer;
        }

        // Draw the data with Direct2D
        m_pDrawDepth->Draw(reinterpret_cast<BYTE*>(m_pDepthRGBX), cDepthWidth * cDepthHeight * sizeof(RGBQUAD));
//      m_depthBuffer[i++] = depth;
    }
#endif
}

/// Copy the color image from the Kinect stream into the color buffer.
void DepthCameraKinectSDK2::_ProcessColor()
{
    ///@todo
    //memcpy(&m_colorBuffer[0], static_cast<BYTE *>(LockedRect.pBits), LockedRect.size);
}

void DepthCameraKinectSDK2::UpdateFrame()
{
    if (!m_pDepthFrameReader)
    {
        return;
    }

    IDepthFrame* pDepthFrame = NULL;

    HRESULT hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);

    if (SUCCEEDED(hr))
    {
        INT64 nTime = 0;
        IFrameDescription* pFrameDescription = NULL;
        int nWidth = 0;
        int nHeight = 0;
        USHORT nDepthMinReliableDistance = 0;
        USHORT nDepthMaxDistance = 0;
        UINT nBufferSize = 0;
        UINT16 *pBuffer = NULL;

        hr = pDepthFrame->get_RelativeTime(&nTime);

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
        }

        if (SUCCEEDED(hr))
        {
            hr = pFrameDescription->get_Width(&nWidth);
        }

        if (SUCCEEDED(hr))
        {
            hr = pFrameDescription->get_Height(&nHeight);
        }

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
        }

        if (SUCCEEDED(hr))
        {
            // In order to see the full range of depth (including the less reliable far field depth)
            // we are setting nDepthMaxDistance to the extreme potential depth threshold
            nDepthMaxDistance = USHRT_MAX;

            // Note:  If you wish to filter by reliable depth distance, uncomment the following line.
            //// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
        }

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
        }

        if (SUCCEEDED(hr))
        {
            _ProcessDepth(pBuffer);
            //_ProcessColor();
            //ProcessDepth(nTime, pBuffer, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxDistance);
        }

        if (pFrameDescription)
            pFrameDescription->Release();
    }
    if (pDepthFrame)
        pDepthFrame->Release();
}

void DepthCameraKinectSDK2::_ScaleDepthMinMax()
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

#endif // USE_KINECTSDK2
