#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
    float nearClip;
    float farClip;
};

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;

vec3 verts[] = {
    {-0.100000, -0.100000, -0.100000},
    {-0.100000, 1.100000, -0.100000},
    {-0.100000, 1.100000, 1.100000},
    {-0.100000, -0.100000, -0.100000},
    {-0.100000, 1.100000, 1.100000},
    {-0.100000, -0.100000, 1.100000},
    {1.100000, -0.100000, -0.100000},
    {1.100000, 1.100000, 1.100000},
    {1.100000, 1.100000, -0.100000},
    {1.100000, -0.100000, -0.100000},
    {1.100000, -0.100000, 1.100000},
    {1.100000, 1.100000, 1.100000},
    {-0.100000, -0.100000, -0.100000},
    {1.100000, 1.100000, -0.100000},
    {-0.100000, 1.100000, -0.100000},
    {-0.100000, -0.100000, -0.100000},
    {1.100000, -0.100000, -0.100000},
    {1.100000, 1.100000, -0.100000},
    {-0.100000, -0.100000, 1.100000},
    {-0.100000, 1.100000, 1.100000},
    {1.100000, 1.100000, 1.100000},
    {-0.100000, -0.100000, 1.100000},
    {1.100000, 1.100000, 1.100000},
    {1.100000, -0.100000, 1.100000},
    {-0.100000, -0.100000, -0.100000},
    {-0.100000, -0.100000, 1.100000},
    {1.100000, -0.100000, 1.100000},
    {-0.100000, -0.100000, -0.100000},
    {1.100000, -0.100000, 1.100000},
    {1.100000, -0.100000, -0.100000},
    {-0.100000, 1.100000, -0.100000},
    {1.100000, 1.100000, 1.100000},
    {-0.100000, 1.100000, 1.100000},
    {-0.100000, 1.100000, -0.100000},
    {1.100000, 1.100000, -0.100000},
    {1.100000, 1.100000, 1.100000},
};

void main() {
    vec3 pos = verts[gl_VertexIndex];
    gl_Position = proj * view * model * vec4(pos, 1.0);
    fragColor = vec3(1.0, 0.0, 0.0);
    fragPos = pos;
}
