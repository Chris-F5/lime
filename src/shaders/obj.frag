#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
};

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
};

layout(set = 1, binding = 1, r8ui) uniform uimage3D voxColors;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    mat4 inverseView = inverse(view);
    vec3 scale = vec3(model[0][0], model[1][1], model[2][2]);
    vec3 camPos = vec3(inverseView[3][0], inverseView[3][1], inverseView[3][2]);
    vec3 fragWorldPos = vec3(model * vec4(fragPos, 1));
    vec3 dir = fragWorldPos - camPos;
    vec3 fragObjPos = fragPos * scale;
    ivec3 fragObjPosInt = ivec3(
        clamp(floor(fragObjPos.x), 0, scale[0] - 1),
        clamp(floor(fragObjPos.y), 0, scale[1] - 1),
        clamp(floor(fragObjPos.z), 0, scale[2] - 1));

    uvec4 imgDat = imageLoad(voxColors, fragObjPosInt);
    uint color = imgDat.x;
    if (color == 0) {
        outColor = vec4(0.0, 0.0, 1.0, 1.0);
    } else {
        outColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    //gl_FragDepth gl_FragCoord.z
}
