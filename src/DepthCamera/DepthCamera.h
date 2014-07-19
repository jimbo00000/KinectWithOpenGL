// DepthCamera.h

#pragma once

#include <vector>

static const float DEFAULT_KINECT_HEIGHT = 1.79f; ///< in meters from the floor
static const float DEFAULT_KINECT_ALPHA = -0.2868f; ///< in radians, negative for below horizontal

class DepthCamera
{
public:
    DepthCamera()
    : m_depthWidth(0)
    , m_depthHeight(0)
    , m_depthBuffer()
    , m_colorBuffer()
    , m_fps(30)
    , m_deviceHeight(DEFAULT_KINECT_HEIGHT)
    , m_alpha(DEFAULT_KINECT_ALPHA)
    //, m_currentStatus(Uninitialized)
    //, m_currentBackend(None)
    {
    }
    virtual ~DepthCamera() {}

    void _AllocateDepthAndColorBuffers()
    {
        m_depthBuffer.resize(m_depthWidth * m_depthHeight);
        m_colorBuffer.resize(4 * m_colorWidth * m_colorHeight);
    }

    virtual void UpdateFrame() {}

    const unsigned int GetDepthWidth() const { return m_depthWidth; } ///< Depth image w
    const unsigned int GetDepthHeight() const { return m_depthHeight; } ///< Depth image h
    const unsigned int GetColorWidth() const { return m_colorWidth; } ///< Color image w
    const unsigned int GetColorHeight() const { return m_colorHeight; } ///< Color image h

    const std::vector<unsigned short>& GetDepthBuffer() const { return m_depthBuffer; }
    const std::vector<unsigned char>& GetColorBuffer() const { return m_colorBuffer; }

    virtual int SaveColorToFile(const std::string& filename);
    virtual int SaveDepthToFile(const std::string& filename);

protected:
    unsigned int m_depthWidth;  ///< width of kinect depth stream buffer
    unsigned int m_depthHeight; ///< height of kinect depth stream buffer
    std::vector<unsigned short> m_depthBuffer;  ///< Pointer to internal libFreenect depth buffer
    
    unsigned int m_colorWidth;  ///< width of kinect color stream buffer
    unsigned int m_colorHeight; ///< height of kinect color stream buffer
    std::vector<unsigned char> m_colorBuffer;  ///< Pointer to color values from camera

    unsigned int m_fps;
    float m_deviceHeight; ///< Height of physical kinect device above the floor
    float m_alpha;        ///< Pitch angle of physical Kinect device

private: // Disallow copy ctor and assignment operator
    DepthCamera(const DepthCamera&);
    DepthCamera& operator=(const DepthCamera&);
};
