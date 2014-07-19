// DepthCamScene.h

#pragma once

#ifdef _WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#endif
#include <GL/glew.h>

#include <glm/glm.hpp>

#include "ShaderWithVariables.h"

class DepthCamScene
{
public:
    DepthCamScene();
    virtual ~DepthCamScene();

    void initGL();
    void timestep(float dt);
    void RenderForOneEye(const float* pMview, const float* pPersp, const float* pObject=NULL) const;

//protected:
    void DrawPoints() const;
    void _InitPointAttributes();

    ShaderWithVariables m_depthpoints;
    GLuint m_depthTex;
    GLuint m_colorTex;
    int m_gridW;
    int m_gridH;

private: // Disallow copy ctor and assignment operator
    DepthCamScene(const DepthCamScene&);
    DepthCamScene& operator=(const DepthCamScene&);
};
