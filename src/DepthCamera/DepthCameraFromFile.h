// DepthCameraFromFile.h

#pragma once

#include "DepthCamera.h"
#include <fstream>

class DepthCameraFromFile : public DepthCamera
{
public:
    DepthCameraFromFile()
    {
#if defined(USE_KINECTSDK)
        _LoadDepth("depthkw.raw");
        _LoadColor("colorkw.raw");
#else
        _LoadDepth("depthni.raw");
        _LoadColor("colorni.raw");
#endif
    }
    virtual ~DepthCameraFromFile() {}
    virtual void UpdateFrame() {}

protected:
    virtual int _LoadDepth(const std::string& filename)
    {
        std::ifstream fs(filename.c_str(), std::ifstream::binary);
        if (!fs.is_open())
        {
            printf("Error : File %s could not be opened.\n", filename.c_str());
            return 1;
        }

        // Read the dimensions
        int dx=0, dy=0;
        fs.read((char*)&dx, sizeof(int));
        fs.read((char*)&dy, sizeof(int));
        m_depthWidth = dx;
        m_depthHeight = dy;

        const size_t sz = dx * dy;
        m_depthBuffer.resize(sz);
        fs.read((char*)&m_depthBuffer[0], sz*sizeof(unsigned short));
        fs.close();

#if 0
        // Post-process into usable range
        for (int i=0; i<sz; ++i)
        {
            m_depthBuffer[i] <<= 3;
        }
#endif
        return 0;
    }

    virtual int _LoadColor(const std::string& filename)
    {
        std::ifstream fs(filename.c_str(), std::ifstream::binary);
        if (!fs.is_open())
        {
            printf("Error : File %s could not be opened.\n", filename.c_str());
            return 1;
        }

        // Read the dimensions
        int dx=0, dy=0;
        fs.read((char*)&dx, sizeof(int));
        fs.read((char*)&dy, sizeof(int));
        m_colorWidth = dx;
        m_colorHeight = dy;

        ///@note KinectSDK has 4 components, OpenNI 3
        const size_t sz = dx * dy * 4;
        m_colorBuffer.resize(sz);
        fs.read((char*)&m_colorBuffer[0], sz*sizeof(unsigned char));
        fs.close();
        return 0;
    }

private: // Disallow copy ctor and assignment operator
    DepthCameraFromFile(const DepthCameraFromFile&);
    DepthCameraFromFile& operator=(const DepthCameraFromFile&);
};
