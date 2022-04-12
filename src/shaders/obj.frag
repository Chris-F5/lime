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

layout(set = 1, binding = 1, r8ui) uniform uimage3D voxColors;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void writeColor(vec3 color) {
    //outColor = vec4(color, 1.0);
}

void writeDepth(float depth) {
    gl_FragDepth = depth;
    outColor = vec4(depth * 40, depth * 40, depth * 40, 1.0);
}

void main() {
    vec3 scale = vec3(model[0][0], model[1][1], model[2][2]);
    mat4 inverseView = inverse(view);
    vec3 camPos = vec3(inverseView[3][0], inverseView[3][1], inverseView[3][2]);

    vec3 modelTranslation = vec3(model[3][0], model[3][1], model[3][2]);

    float baseDistance;
    vec3 objPos;
    ivec3 objPosInt;
    if (gl_FrontFacing) {
        objPos = fragPos * scale;
        baseDistance = gl_FragCoord.z;
    } else {
        // todo: check if camera in object
        objPos = camPos - modelTranslation;
        baseDistance = 0;
    }
    objPosInt = ivec3(
        floor(objPos.x),
        floor(objPos.y),
        floor(objPos.z));

    // todo: find dir faster
    vec3 fragWorldPos = vec3(model * vec4(fragPos, 1));
    vec3 dir = fragWorldPos - camPos;
    dir = normalize(dir);

    uint hit = 1;
    vec3 hitNormal;

    uvec4 imgDat = imageLoad(voxColors, objPosInt);
    if (imgDat.x != 0) {
        writeColor(vec3(1.0, 0.0, 1.0));
        writeDepth(0.0);
        return;
    }

    ivec3 step;
    ivec3 outOfBounds;
    vec3 tDelta;
    vec3 tMax;
    if (dir.x >= 0) {
        step.x = 1;
        outOfBounds.x = int(scale.x);
        tDelta.x = 1 / dir.x;
        tMax.x = tDelta.x * (objPosInt.x + 1 - objPos.x);
        if (objPosInt.x >= outOfBounds.x) {
            hit = 0;
        }
    } else {
        step.x = -1;
        outOfBounds.x = -1;
        tDelta.x = 1 / -dir.x;
        tMax.x = tDelta.x * (objPos.x - objPosInt.x);
        if (objPosInt.x <= outOfBounds.x) {
            hit = 0;
        }
    }

    if (dir.y >= 0) {
        step.y = 1;
        outOfBounds.y = int(scale.y);
        tDelta.y = 1 / dir.y;
        tMax.y = tDelta.y * (objPosInt.y + 1 - objPos.y);
        if (objPosInt.y >= outOfBounds.y) {
            hit = 0;
        }
    } else {
        step.y = -1;
        outOfBounds.y = -1;
        tDelta.y = 1 / -dir.y;
        tMax.y = tDelta.y * (objPos.y - objPosInt.y);
        if (objPosInt.y <= outOfBounds.y) {
            hit = 0;
        }
    }

    if (dir.z >= 0) {
        step.z = 1;
        outOfBounds.z = int(scale.z);
        tDelta.z = 1 / dir.z;
        tMax.z = tDelta.z * (objPosInt.z + 1 - objPos.z);
        if (objPosInt.z >= outOfBounds.z) {
            hit = 0;
        }
    } else {
        step.z = -1;
        outOfBounds.z = -1;
        tDelta.z = 1 / -dir.z;
        tMax.z = tDelta.z * (objPos.z - objPosInt.z);
        if (objPosInt.z <= outOfBounds.z) {
            hit = 0;
        }
    }

    float t = 0;
    if (hit == 1) {
        hit = 0;
        while(true) {
            if(tMax.x < tMax.y) {
                if (tMax.x < tMax.z) {
                    objPosInt.x += step.x;
                    if (objPosInt.x == outOfBounds.x) {
                        break;
                    }
                    t = tMax.x;
                    tMax.x += tDelta.x;
                    hitNormal = vec3(-step.x, 0.0, 0.0);
                } else {
                    objPosInt.z += step.z;
                    if (objPosInt.z == outOfBounds.z) {
                        break;
                    }
                    t = tMax.z;
                    tMax.z += tDelta.z;
                    hitNormal = vec3(0.0, 0.0, -step.z);
                }
            } else {
                if (tMax.y < tMax.z) {
                    objPosInt.y += step.y;
                    if (objPosInt.y == outOfBounds.y) {
                        break;
                    }
                    t = tMax.y;
                    tMax.y += tDelta.y;
                    hitNormal = vec3(0.0, -step.y, 0);
                } else {
                    objPosInt.z += step.z;
                    if (objPosInt.z == outOfBounds.z) {
                        break;
                    }
                    t = tMax.z;
                    tMax.z += tDelta.z;
                    hitNormal = vec3(0.0, 0.0, -step.z);
                }
            }

            uvec4 imgDat = imageLoad(voxColors, objPosInt);
            if (imgDat.x != 0) {
                hit = imgDat.x;
                break;
            }
        }
    }

    if (hit == 0) {
        writeColor(vec3(0.0, 0.0, 1.0));
        writeDepth(1);
    } else {
        vec3 lightDir = vec3(0.2, 0.7, 0.1);
        float normalLight = dot(lightDir, hitNormal);
        writeColor(vec3(0.8 * normalLight + 0.2, 0.0, 0.0));

        if (baseDistance == 0) {
            writeDepth(
                (t - nearClip)
                / (farClip - nearClip));
        } else {
            writeDepth(
                baseDistance
                + t / (farClip - nearClip));
        }
    }
}
