#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
    float nearClip;
    float farClip;
    uint time;
    uint movedThisFrame;
};

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uint outSurfaceId;

void main()
{
    gl_FragDepth = 0.999;
    outAlbedo = vec4(inColor, 1.0);
    outNormal = vec4(0.0, 1.0, 0.0, 1.0);
    outSurfaceId = 1;
}
