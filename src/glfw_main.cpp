// glfw_main.cpp

#include <GL/glew.h>

#if defined(_WIN32)
#  include <Windows.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <string.h>
#include <sstream>
#include <pthread.h>

#ifdef USE_ANTTWEAKBAR
#  include <AntTweakBar.h>
#endif

#include "ShaderWithVariables.h"
#include "FPSTimer.h"
#include "DepthCameraKinectSDK.h"
#include "DepthCameraKinectSDK2.h"
#include "DepthCameraOpenNI.h"
#include "DepthCameraFromFile.h"
#include "DepthCamScene.h"

// mouse motion internal state
int oldx, oldy, newx, newy;
int which_button = -1;
int modifier_mode = 0;

GLFWwindow* g_pMonoWindow = NULL;
GLFWwindow* g_pUploadWindow = NULL;

ShaderWithVariables g_presentDepth;
ShaderWithVariables g_presentTex;
FPSTimer g_fps;
FPSTimer g_uploadTimer;

///@todo Encapsulate in a class
DepthCamera* g_pCamera = NULL;
GLuint g_tex = 0;
GLuint g_texDepth = 0;
int g_texw = 640;
int g_texh = 480;
int g_texDepthw = 80;
int g_texDepthh = 60;
pthread_t g_captureThread;
unsigned int g_captureThreadSleepMs = 10;

DepthCamScene g_scene;
glm::vec2 g_rotate(0.0f, 0.0f);
glm::vec3 g_translate(0.0f, 0.0f, 0.0f);
float g_depthScale = 1.0f;
unsigned int g_depthScaleRaw = 0xFFFF;

#ifdef USE_ANTTWEAKBAR
TwBar* g_pTweakbar = NULL;
#endif


static void ErrorCallback(int p_Error, const char* p_Description)
{
    printf("ERROR: %d, %s\n", p_Error, p_Description);
}


void keyboard(GLFWwindow*, int key, int, int action, int)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        default: break;

        case '-':
            if (g_captureThreadSleepMs > 0)
                --g_captureThreadSleepMs;
            break;

        case '=':
            ++g_captureThreadSleepMs;
            break;

        case ' ':
            if (g_pCamera != NULL)
            {
#if defined(USE_KINECTSDK)
                g_pCamera->SaveDepthToFile("depthkw.raw");
                g_pCamera->SaveColorToFile("colorkw.raw");
#else
                g_pCamera->SaveDepthToFile("depthni.raw");
                g_pCamera->SaveColorToFile("colorni.raw");
#endif
            }
            break;

        case GLFW_KEY_ESCAPE:
            exit(0);
            break;
        }
    }
}

void mouseDown(GLFWwindow* pWindow, int button, int action, int)
{
#ifdef USE_ANTTWEAKBAR
    int ant = TwEventMouseButtonGLFW(button, action);
    if (ant != 0)
        return;
#endif

    double xd, yd;
    glfwGetCursorPos(pWindow, &xd, &yd);
    const int x = static_cast<int>(xd);
    const int y = static_cast<int>(yd);

    which_button = button;
    oldx = newx = x;
    oldy = newy = y;
    if (action == GLFW_RELEASE)
    {
        which_button = -1;
    }

}

void mouseMove(GLFWwindow* pWindow, double xd, double yd)
{
    glfwGetCursorPos(pWindow, &xd, &yd);
    const int x = static_cast<int>(xd);
    const int y = static_cast<int>(yd);

    oldx = newx;
    oldy = newy;
    newx = x;
    newy = y;
    const int mmx = x-oldx;
    const int mmy = y-oldy;

    if (which_button == GLFW_MOUSE_BUTTON_1)
    {
        const float spinMagnitude = 0.01f;
        g_rotate.x += static_cast<float>(mmx) * spinMagnitude;
        g_rotate.y += static_cast<float>(mmy) * spinMagnitude;
    }
    else if (which_button == GLFW_MOUSE_BUTTON_2)
    {
        const float moveMagnitude = 0.01f;
        g_translate.x += static_cast<float>(mmx) * moveMagnitude;
        g_translate.z += static_cast<float>(mmy) * moveMagnitude;
    }
    else if ((which_button == GLFW_MOUSE_BUTTON_3) ||
             (which_button == GLFW_MOUSE_BUTTON_4))
    {
        const float moveMagnitude = 0.01f;
        g_translate.y -= static_cast<float>(mmy) * moveMagnitude;
    }

#ifdef USE_ANTTWEAKBAR
    TwEventMousePosGLFW(static_cast<int>(xd), static_cast<int>(yd));
#endif
}

void mouseWheel(GLFWwindow* pWindow, double, double)
{
    (void)pWindow;
}

void resize(GLFWwindow* pWindow, int, int)
{
    (void)pWindow;
}

#ifdef USE_ANTTWEAKBAR

static void TW_CALL GetDisplayFPS(void *value, void *clientData)
{
    *(unsigned int *)value = static_cast<unsigned int>(g_fps.GetFPS());
}

static void TW_CALL GetUploadFPS(void *value, void *clientData)
{
    *(unsigned int *)value = static_cast<unsigned int>(g_uploadTimer.GetFPS());
}

void InitializeBar()
{
    ///@note Bad size errors will be thrown if this is not called at init
    TwWindowSize(1000, 800);

    g_pTweakbar = TwNewBar("TweakBar");

    TwAddVarCB(g_pTweakbar, "Display FPS", TW_TYPE_UINT32, NULL, GetDisplayFPS, NULL, " group='Performance' ");
    TwAddVarCB(g_pTweakbar, "Upload FPS", TW_TYPE_UINT32, NULL, GetUploadFPS, NULL, " group='Performance' ");

    TwAddVarRW(g_pTweakbar, "Thread sleep", TW_TYPE_UINT32, &g_captureThreadSleepMs,
               " min=1 max=100 help='Thread sleep interval in ms' ");
    TwAddVarRW(g_pTweakbar, "Depth Scale", TW_TYPE_FLOAT, &g_depthScale,
               " min=0 max=100 step=0.05 help='Depth scale factor' ");
    TwAddVarRW(g_pTweakbar, "Depth Scale Raw", TW_TYPE_UINT32, &g_depthScaleRaw,
               " min=0 max=3276800 step=256 help='Depth scale factor' ");
}
#endif


void initTexShader(ShaderWithVariables& swv, bool use_ll=true)
{
    swv.initProgram("simpletex");
    swv.bindVAO();

    float m = -0.9f;
    const float s = 0.3f;
    const float vertsLL[] = {
        m  , m,
        m+s, m,
        m+s, m+s,
        m  , m+s,
    };

    const float n = -0.5f;
    const float vertsRR[] = {
        n  , m,
        n+s, m,
        n+s, m+s,
        n  , m+s,
    };

    const float texs[] = {
        0, 1,
        1, 1,
        1, 0,
        0, 0,
    };

    GLuint vertVbo = 0;
    glGenBuffers(1, &vertVbo);
    swv.AddVbo("vPosition", vertVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
    glBufferData(GL_ARRAY_BUFFER, 4*2*sizeof(GLfloat), use_ll?vertsLL:vertsRR, GL_STATIC_DRAW);
    glVertexAttribPointer(swv.GetAttrLoc("vPosition"), 2, GL_FLOAT, GL_FALSE, 0, NULL);

    GLuint texVbo = 0;
    glGenBuffers(1, &texVbo);
    swv.AddVbo("vTex", texVbo);
    glBindBuffer(GL_ARRAY_BUFFER, texVbo);
    glBufferData(GL_ARRAY_BUFFER, 4*2*sizeof(GLfloat), texs, GL_STATIC_DRAW);
    glVertexAttribPointer(swv.GetAttrLoc("vTex"), 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(swv.GetAttrLoc("vPosition"));
    glEnableVertexAttribArray(swv.GetAttrLoc("vTex"));

    glUseProgram(swv.prog());
    {
        const glm::mat4 id(1.0f);
        glUniformMatrix4fv(swv.GetUniLoc("mvmtx"), 1, false, glm::value_ptr(id));
        glUniformMatrix4fv(swv.GetUniLoc("prmtx"), 1, false, glm::value_ptr(id));
    }
    glUseProgram(0);

    glBindVertexArray(0);
}

void initTextures()
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLuint tex = 0;
    // Texture render target
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ///@note Could use glTexStorage2D here...
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    g_tex = tex;


    // Depth render target
    GLuint texd = 0;
    glGenTextures(1, &texd);
    glBindTexture(GL_TEXTURE_2D, texd);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        ///@note Could use glTexStorage2D here...
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    g_texDepth = texd;
    g_scene.m_depthTex = texd;
}

void presentTex(const ShaderWithVariables& swv, const GLuint tex)
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    const GLuint prog = swv.prog();
    glUseProgram(prog);
    swv.bindVAO();
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(swv.GetUniLoc("tex"), 0);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

void uploadTexture()
{
    if (g_pCamera == NULL)
        return;

    g_pCamera->UpdateFrame();

    ///@note KinectSDK has 4 components, OpenNI 3
#if USE_KINECTSDK
    const GLint internalFormat = GL_RGBA;
    const GLint format = GL_BGRA;
#else
    const GLint internalFormat = GL_RGB;
    const GLint format = GL_RGB;
#endif
    
    g_texw = g_pCamera->GetColorWidth();
    g_texh = g_pCamera->GetColorHeight();
    g_texDepthw = g_pCamera->GetDepthWidth();
    g_texDepthh = g_pCamera->GetDepthHeight();

    const std::vector<unsigned char>& col = g_pCamera->GetColorBuffer();
    if (!col.empty())
    {
        const int tex = g_tex;
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                        g_texw, g_texh, 0,
                        format, GL_UNSIGNED_BYTE,
                        &col[0]);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    const std::vector<unsigned short>& dep = g_pCamera->GetDepthBuffer();
    if (!dep.empty())
    {
        const int texd = g_texDepth;
        // Texture upload
        glBindTexture(GL_TEXTURE_2D, texd);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                        g_texDepthw, g_texDepthh, 0,
                        GL_LUMINANCE, GL_UNSIGNED_SHORT,
                        &dep[0]);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

// The function that runs in another thread
void *ThreadFunction(void*)
{
    if (g_pUploadWindow != NULL)
    {
        // Make this context current for this thread once at the beginning, then we can
        // upload the image as often as the loop runs.
        glfwMakeContextCurrent(g_pUploadWindow);
    }

    g_pCamera = 
#if defined(USE_SAVED_DATA)
        new DepthCameraFromFile();
#elif defined(USE_KINECTSDK2)
        new DepthCameraKinectSDK2();
#elif defined(USE_KINECTSDK)
        new DepthCameraKinectSDK();
#elif defined(USE_OPENNI)
        new DepthCameraOpenNI();
#else
        NULL;
#endif

    while (true)
    {
        if (g_pCamera != NULL)
            g_pCamera->m_depthScale = g_depthScaleRaw;
        g_uploadTimer.OnFrame();
        uploadTexture();

#if defined(_WIN32)
        ::Sleep(g_captureThreadSleepMs);
#endif
    }

    pthread_exit(NULL);
    return NULL;
}

void display()
{
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // draw 3d scene
    const glm::vec3 EyePos(0.0f, 0.0f, 1.0f);
    const glm::vec3 LookVec(0.0f, 0.0f, -1.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);

    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::translate(rotation, g_translate);
    rotation = glm::rotate(rotation, g_rotate.x, glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, g_rotate.y, glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::scale(rotation, glm::vec3(1.0f, 1.0f, g_depthScale));

    const glm::mat4 lookat = glm::lookAt(EyePos, EyePos + LookVec, up);

    const glm::mat4 modelview = lookat * rotation;

    const int w = 1000;
    const int h = 800;
    glm::mat4 persp = glm::perspective(
        90.0f,
        static_cast<float>(w)/static_cast<float>(h),
        0.004f,
        500.0f);

    g_scene.RenderForOneEye(glm::value_ptr(modelview), glm::value_ptr(persp), NULL);

    presentTex(g_presentDepth, g_texDepth);
    presentTex(g_presentTex, g_tex);

#ifdef USE_ANTTWEAKBAR
    TwRefreshBar(g_pTweakbar);
    TwDraw(); ///@todo Should this go first? Will it write to a depth buffer?
#endif
}

void printGLContextInfo(GLFWwindow* pW)
{
    // Print some info about the OpenGL context...
    const int l_Major = glfwGetWindowAttrib(pW, GLFW_CONTEXT_VERSION_MAJOR);
    const int l_Minor = glfwGetWindowAttrib(pW, GLFW_CONTEXT_VERSION_MINOR);
    const int l_Profile = glfwGetWindowAttrib(pW, GLFW_OPENGL_PROFILE);
    if (l_Major >= 3) // Profiles introduced in OpenGL 3.0...
    {
        if (l_Profile == GLFW_OPENGL_COMPAT_PROFILE)
        {
            printf("GLFW_OPENGL_COMPAT_PROFILE\n");
        }
        else
        {
            printf("GLFW_OPENGL_CORE_PROFILE\n");
        }
    }
    printf("OpenGL: %d.%d ", l_Major, l_Minor);
    printf("Vendor: %s\n", (char*)glGetString(GL_VENDOR));
    printf("Renderer: %s\n", (char*)glGetString(GL_RENDERER));
}

int main(void)
{
    ///@todo Command line options

    GLFWwindow* l_Window = NULL;

    glfwSetErrorCallback(ErrorCallback);

    if (!glfwInit())
    {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_SAMPLES, 0);
    l_Window = glfwCreateWindow(1000, 800, "Kinect Threaded Context", NULL, NULL);

    if (!l_Window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(l_Window);
    glfwSetWindowSizeCallback(l_Window, resize);
    glfwSetMouseButtonCallback(l_Window, mouseDown);
    glfwSetCursorPosCallback(l_Window, mouseMove);
    glfwSetScrollCallback(l_Window, mouseWheel);
    glfwSetKeyCallback(l_Window, keyboard);

    // Don't forget to initialize Glew, turn glewExperimental on to
    // avoid problems fetching function pointers...
    glewExperimental = GL_TRUE;
    const GLenum l_Result = glewInit();
    if (l_Result != GLEW_OK)
    {
        printf("glewInit() error.\n");
        exit(EXIT_FAILURE);
    }

    printGLContextInfo(l_Window);

    glfwMakeContextCurrent(l_Window);
    g_pMonoWindow = l_Window;

    initTexShader(g_presentDepth);
    initTexShader(g_presentTex, false);
    initTextures();

#ifdef USE_ANTTWEAKBAR
    TwInit(TW_OPENGL, NULL);
    InitializeBar();
#endif

    g_scene.initGL();

    // Create a second context for upload - no visible window necessary.
    glfwWindowHint(GLFW_VISIBLE, 0);
    g_pUploadWindow = glfwCreateWindow(100, 100, "Upload Context", NULL, g_pMonoWindow);

    // Start capture thread
    const int res = pthread_create(&g_captureThread, NULL, ThreadFunction, NULL);
    if (res)
    {
        printf("pthread_create failed\n");
    }

    while (!glfwWindowShouldClose(l_Window))
    {
        glfwPollEvents();
        g_fps.OnFrame();

        display();

        glfwSwapBuffers(g_pMonoWindow);

#if 0 //ndef _LINUX
        // Indicate FPS in window title
        // This is absolute death for performance in Ubuntu Linux 12.04
        {
            std::ostringstream oss;
            oss << "GLFW Oculus Rift Test - "
                << static_cast<int>(g_fps.GetFPS())
                << " fps display, "
                << static_cast<int>(g_uploadTimer.GetFPS())
                << " upload";
            glfwSetWindowTitle(l_Window, oss.str().c_str());
            if (g_pMonoWindow != NULL)
                glfwSetWindowTitle(g_pMonoWindow, oss.str().c_str());
        }
#endif
    }

    glfwDestroyWindow(l_Window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
}
