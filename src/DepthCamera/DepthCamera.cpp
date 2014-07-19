// DepthCamera.cpp

#include "DepthCamera.h"
#include <fstream>

int DepthCamera::SaveDepthToFile(const std::string& filename)
{
    std::ofstream fs(filename.c_str(), std::ofstream::binary);
    if (!fs.is_open())
    {
        // Error: file could not be opened
        return 1;
    }

    const int dw = GetDepthWidth();
    const int dh = GetDepthHeight();
    fs.write((char*)&dw, sizeof(int));
    fs.write((char*)&dh, sizeof(int));
    fs.write((char*)&GetDepthBuffer()[0], dw*dh*sizeof(unsigned short));

    fs.close();
    return 0;
}

int DepthCamera::SaveColorToFile(const std::string& filename)
{
    std::ofstream fs(filename.c_str(), std::ofstream::binary);
    if (!fs.is_open())
    {
        // Error: file could not be opened
        return 1;
    }

    const int w = GetColorWidth();
    const int h = GetColorHeight();
    fs.write((char*)&w, sizeof(int));
    fs.write((char*)&h, sizeof(int));
    ///@note KinectSDK has 4 components, OpenNI 3
    fs.write((char*)&GetColorBuffer()[0], w*h*4*sizeof(unsigned char));

    fs.close();
    return 0;
}
