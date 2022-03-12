#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
} global;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in uint objRef;

layout(location = 0) out vec3 fragColor;

vec3 positions[3] = vec3[](
    vec3(0.0, 5, 10.0),
    vec3(5.0, -5.0, 10.0),
    vec3(-5.0, -5.0, 10.0)
);


void main() {
    gl_Position = global.proj * global.view * inPosition;
    if(objRef == 0) {
        fragColor = vec3(1.0, 0.0, 0.0);
    } else if (objRef == 1) {
        fragColor = vec3(0.0, 1.0, 0.0);
    } else {
        fragColor = vec3(1.0, 0.0, 1.0);
    }
}
