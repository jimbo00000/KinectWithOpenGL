// depthpoints.vert
#version 130

attribute vec3 vPosition;

varying vec2 vfTex;

uniform mat4 mvmtx;
uniform mat4 prmtx;

uniform sampler2D u_sampDepth;

void main()
{
    vec2 ndcPos = vPosition.xy;
    vec2 in_xy = vec2(0.5) + 0.5*(ndcPos);
    vfTex = in_xy;

    // This is where the magic should happen - sample the depth texture directly from the vertex shader
    // and set each point's location according to its nearest sampled depth value.
    vec4 texel = textureLod(u_sampDepth, in_xy, 0.0);
    float depthValue = texel.r;

    vec3 pos = vPosition;
    pos.z = depthValue;
    // Push all points of unknown depth away where they shouldn't be visible.
    if (depthValue == 0.0)
    {
        pos = vec3(9999.0);
    }
    gl_Position = prmtx * mvmtx * vec4(pos, 1.0);



#if 0
//http://visheshvision.com/2014/04/28/transformation-matrix-used-in-kinect-for-windows-to-transform-depth-image-to-point-cloud/
    float d = texel.z * 5000 + 3000;
    vec3 imgpt = vec3(640*in_xy.x, 480*in_xy.y, d);
    float k = d * 8;
    const float m = 0.000219;
    const float c = -0.00007002;
    const float g = 0.000052515;
    mat4 pointmtx = mat4(
        m,  0,   c, 0,
        0, -m,   g, 0,
        0,  0, 1/k, 0,
        0,  0,   0, 1);
    vec4 worldpt = pointmtx * vec4(imgpt, 1.0);
    gl_Position = prmtx * mvmtx * worldpt;
#endif


#if 0
    //https://code.google.com/p/stevenhickson-code/source/browse/trunk/blepo/external/Microsoft/Kinect/NuiSkeleton.h?r=14
    vec2 imgpt = vec2(640*in_xy.x, 480*in_xy.y);
    const int width = 640;
    const int height = 480;
    const int USHRT_MAX = 65535;
    const float NUI_MULTI = 3.501e-3;
    float fSkeletonZ = 10 * texel.z; //(float(USHRT_MAX) * texel.z) / 1000.0;
    float skelx = (imgpt.x - width/2.0) * (320.0/width) * NUI_MULTI * fSkeletonZ;
    float skely = -(imgpt.y - height/2.0) * (240.0/height) * NUI_MULTI * fSkeletonZ;

    pos.x = skelx;
    pos.y = skely;
    pos.z = fSkeletonZ;

    gl_Position = prmtx * mvmtx * vec4(pos, 1.0);
#endif
}
