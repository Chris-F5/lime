#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
};

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    fragColor = vec3(1.0, 0.0, 0.0);
    fragPos = inPosition;

    /*
    if(gl_Position.y > 0.5) {
        fragColor = vec3(1.0, 0.0, 1.0);
    }
    */
}
