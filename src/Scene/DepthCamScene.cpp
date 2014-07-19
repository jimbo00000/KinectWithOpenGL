// DepthCamScene.cpp

#include "DepthCamScene.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>

DepthCamScene::DepthCamScene()
: m_depthpoints()
, m_depthTex(0)
, m_colorTex(0)
, m_gridW(640)
, m_gridH(480)
{
}

DepthCamScene::~DepthCamScene()
{
}

void DepthCamScene::initGL()
{
    m_depthpoints.initProgram("depthpoints");
    m_depthpoints.bindVAO();
    _InitPointAttributes();
    glBindVertexArray(0);
}

///@brief While the VAO is bound, gen and bind all buffers and attribs.
void DepthCamScene::_InitPointAttributes()
{
    std::vector<glm::vec3> verts;
    for (int i=0; i<m_gridH; ++i)
    {
        for (int j=0; j<m_gridW; ++j)
        {
            const float fx = static_cast<float>(j) / static_cast<float>(m_gridW);
            const float fy = static_cast<float>(i) / static_cast<float>(m_gridH);
            glm::vec3 pt(-1.0f + 2.0f*fx, -1.0f + 2.0f*fy, 0.0f);
            verts.push_back(pt);
        }
    }
    const int sz = m_gridW * m_gridH;

    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    m_depthpoints.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, sz*3*sizeof(GLfloat), &verts[0], GL_STATIC_DRAW);
    glVertexAttribPointer(m_depthpoints.GetAttrLoc("vPosition"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(m_depthpoints.GetAttrLoc("vPosition"));
}

void DepthCamScene::DrawPoints() const
{
    m_depthpoints.bindVAO();
    glPointSize(1.0f);
    const int sz = m_gridW * m_gridH;
    glDrawArrays(GL_POINTS, 0, sz);
    glBindVertexArray(0);
}

void DepthCamScene::RenderForOneEye(const float* pMview, const float* pPersp, const float* pObject) const
{
    const glm::mat4 modelview = glm::make_mat4(pMview);
    const glm::mat4 projection = glm::make_mat4(pPersp);

    glm::mat4 object = glm::mat4(1.0f);
    if (pObject != NULL)
        object = glm::make_mat4(pObject);

    glUseProgram(m_depthpoints.prog());
    {
        glUniformMatrix4fv(m_depthpoints.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(modelview));
        glUniformMatrix4fv(m_depthpoints.GetUniLoc("prmtx"), 1, false, glm::value_ptr(projection));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_depthTex);
        glUniform1i(m_depthpoints.GetUniLoc("u_sampDepth"), 0);

        DrawPoints();
    }
    glUseProgram(0);
}
