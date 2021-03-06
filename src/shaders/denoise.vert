#version 450

layout (location = 0) out vec2 outUV;

vec2 positions[3] = vec2[](
    vec2(0, 0),
    vec2(2, 0),
    vec2(0, 2)
);

void main() {
    outUV = positions[gl_VertexIndex];
    gl_Position = vec4(outUV * 2.0f - 1.0, 0.0, 1.0);
}
