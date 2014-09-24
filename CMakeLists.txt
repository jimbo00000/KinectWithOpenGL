CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
PROJECT( KinectThreadedContext )

INCLUDE(cmake_modules/HardcodeShaders.cmake)

# Third-party Library settings
SET( GL_FRAMEWORK "GLFW" CACHE STRING "Which OpenGL windowing framework to use" )
SET( USE_ANTTWEAKBAR TRUE CACHE BOOL "Use AntTweakBar" )
SET( USE_SAVED_DATA FALSE CACHE BOOL "Use Saved files of Kinect data" )
IF( USE_SAVED_DATA )
    ADD_DEFINITIONS( -DUSE_SAVED_DATA )
ENDIF( USE_SAVED_DATA )

#
# Platform-dependent section
#
IF( WIN32 )
    #
    # Custom Windows include and link dirs for my machine:
    #
    SET( LIBS_HOME "C:/lib" CACHE STRING "Location of support libraries Glut, Glew and others"  )

    SET( GLM_ROOT "${LIBS_HOME}/glm-0.9.5.4/glm" )
    INCLUDE_DIRECTORIES( "${GLM_ROOT}" )
    ADD_DEFINITIONS( -DGLM_FORCE_RADIANS )

    SET( GLEW_ROOT "${LIBS_HOME}/glew-1.11.0" CACHE STRING "glew root" )
    INCLUDE_DIRECTORIES( "${GLEW_ROOT}/include" )
    LINK_DIRECTORIES   ( "${GLEW_ROOT}/lib/Release/Win32" )
    ADD_DEFINITIONS( -DGLEW_STATIC )

    IF( GL_FRAMEWORK STREQUAL "GLFW" )
        SET( GLFW3_ROOT "${LIBS_HOME}/glfw-3.0.1.bin.WIN32" CACHE STRING "glfw root" )
        INCLUDE_DIRECTORIES( "${GLFW3_ROOT}/include" )
        LINK_DIRECTORIES   ( "${GLFW3_ROOT}/lib-msvc100" )
        SET( FRAMEWORK_LIBS glfw3.lib )
    ELSEIF( GL_FRAMEWORK STREQUAL "SDL" )
        SET( SDL2_ROOT "${LIBS_HOME}/SDL2-2.0.3" CACHE STRING "SDL2 root" )
        INCLUDE_DIRECTORIES( "${SDL2_ROOT}/include" )
        LINK_DIRECTORIES   ( "${SDL2_ROOT}/lib/x86" )
        SET( FRAMEWORK_LIBS SDL2.lib )
    ENDIF()

    # TODO: use c++11 or Boost threads
    SET( PTHREADS_ROOT "${LIBS_HOME}/pthreads/Pre-built.2" )
    INCLUDE_DIRECTORIES( "${PTHREADS_ROOT}/include" )
    LINK_DIRECTORIES   ( "${PTHREADS_ROOT}/lib/x86" )

    #
    # Choose one of the following Kinect backends here by setting its variable to TRUE.
    #
    SET( USE_KINECTSDK FALSE CACHE BOOL "Use Kinect4Windows SDK" )
    SET( USE_OPENNI FALSE CACHE BOOL "Use OpenNI library" )
    SET( USE_LIBFREENECT FALSE )
    
    #
    # Kinect backend specific sections
    #
    IF( USE_KINECTSDK )
        MESSAGE("Using Kinect SDK (USE_KINECTSDK=TRUE)")
        ADD_DEFINITIONS( -DUSE_KINECTSDK )
        SET( KINECTSDK_HOME "C:/Program Files/Microsoft SDKs/Kinect/v1.6/" )

        # <IncludePath>$(KINECTSDK10_DIR)\inc;$(IncludePath)</IncludePath>
        # <LibraryPath>$(KINECTSDK10_DIR)\lib\x86;$(LibraryPath)</LibraryPath>
        INCLUDE_DIRECTORIES("$ENV{KINECTSDK10_DIR}inc/")
        LINK_DIRECTORIES("$ENV{KINECTSDK10_DIR}lib/x86/")
        
        SET( KINECT_LIBS Kinect10.lib )
    ENDIF( USE_KINECTSDK )

    IF( USE_OPENNI )
        MESSAGE("Using OpenNI (USE_OPENNI=TRUE)")
        INCLUDE_DIRECTORIES("$ENV{OPEN_NI_INCLUDE}")
        LINK_DIRECTORIES("$ENV{OPEN_NI_LIB}")
        SET( KINECT_LIBS ${KINECT_LIBS} openNI.lib )
        ADD_DEFINITIONS( -DUSE_OPENNI )
    ENDIF( USE_OPENNI )

    IF( USE_ANTTWEAKBAR )
        SET( ANTTWEAKBAR_ROOT "${LIBS_HOME}/AntTweakBar_116/AntTweakBar" )
        INCLUDE_DIRECTORIES( "${ANTTWEAKBAR_ROOT}/include" )
        LINK_DIRECTORIES   ( "${ANTTWEAKBAR_ROOT}/lib" )
        SET( ANT_LIBS AntTweakBar.lib )
        ADD_DEFINITIONS( -DUSE_ANTTWEAKBAR )
    ENDIF( USE_ANTTWEAKBAR )

    SET( PLATFORM_LIBS
        ${FRAMEWORK_LIBS}
        opengl32.lib glu32.lib glew32s.lib
        pthreadVCE2.lib
        ${KINECT_LIBS}
        )

    ADD_DEFINITIONS( /W4 )

ELSEIF( APPLE )
ELSEIF( UNIX )
    ADD_DEFINITIONS( -D_LINUX )
    SET( LIBS_HOME "~/lib" )
    
    ADD_DEFINITIONS( -D_DEBUG )

    SET( GLM_ROOT "${LIBS_HOME}/glm-0.9.5.4/glm" )
    INCLUDE_DIRECTORIES( "${GLM_ROOT}" )
    ADD_DEFINITIONS( -DGLM_FORCE_RADIANS )

    IF( GL_FRAMEWORK STREQUAL "GLFW" )
        SET( FRAMEWORK_LIBS -lglfw3 )
    ELSEIF( GL_FRAMEWORK STREQUAL "SDL" )
        INCLUDE_DIRECTORIES( "/usr/local/include/SDL2" )
        LINK_DIRECTORIES   ( "/usr/local/lib/x86" )
        SET( FRAMEWORK_LIBS -lSDL2 )
    ENDIF()

    find_package(OpenGL REQUIRED)
    find_package(GLUT REQUIRED)
    include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} )

    SET( PLATFORM_LIBS
        -lGLEW -lGL
        -lGLU
        ${FRAMEWORK_LIBS}
        -pthread
        -ludev
        -lrt
        -lXinerama
        -lXrandr
        -lXxf86vm
        -lXi -lX11
        )
ENDIF()


#
# Platform-independent section
#
FILE( GLOB_RECURSE UTIL_SOURCE_FILES
    src/Util/*.cpp
    src/Util/*.h
    )

FILE( GLOB_RECURSE DEPTHCAMERA_SOURCE_FILES
    src/DepthCamera/*.cpp
    src/DepthCamera/*.h
    )

FILE( GLOB_RECURSE SCENE_SOURCE_FILES
    src/Scene/*.cpp
    src/Scene/*.h
    )

INCLUDE_DIRECTORIES("src/Util")
INCLUDE_DIRECTORIES("src/Util/GL")
INCLUDE_DIRECTORIES("src/DepthCamera")
INCLUDE_DIRECTORIES("src/Scene")

ADD_LIBRARY( Util ${UTIL_SOURCE_FILES} )
ADD_LIBRARY( DepthCamera ${DEPTHCAMERA_SOURCE_FILES} )
ADD_LIBRARY( Scene ${SCENE_SOURCE_FILES} )

IF( GL_FRAMEWORK STREQUAL "GLFW" )
    ADD_EXECUTABLE( ${PROJECT_NAME} src/glfw_main.cpp )
ELSEIF( GL_FRAMEWORK STREQUAL "SDL" )
    ADD_EXECUTABLE( ${PROJECT_NAME} src/sdl_main.cpp )
ENDIF()

TARGET_LINK_LIBRARIES( ${PROJECT_NAME}
    Util
    DepthCamera
    Scene
    ${PLATFORM_LIBS}
    )
