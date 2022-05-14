#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
    float nearClip;
    float farClip;
    uint time;
    uint movedThisFrame;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in uint color;

layout(location = 0) out vec3 outColor;

out gl_PerVertex
{
    vec4 gl_Position;
    float gl_PointSize;
};

void main ()
{
    gl_Position = proj * view * vec4(pos, 1.0);
    gl_PointSize = 8.0;
    outColor = vec3(1.0, 0.0, 1.0);
}
