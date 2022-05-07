#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
    float nearClip;
    float farClip;
    uint time;
    uint movedThisFrame;
};

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
    mat4 model;
};

#define MAX_OBJ_SCALE 256
#define SURFACE_LIGHT_BUFFER_COUNT 1000000

layout(set = 1, binding = 1, r8ui) uniform uimage3D voxColors;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPos;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uint outSurfaceId;

void writeDepth(float depth)
{
    gl_FragDepth = depth;
}

void writeAlbedo(vec3 color)
{
    outAlbedo = vec4(color, 1.0);
}

void writeNormal(vec3 normal)
{
    outNormal = vec4(normal, 1.0);
}

void writeSurfaceId(uint id) {
    outSurfaceId = id;
}

float camDistanceToDepth(float distanceFromCameraPlane)
{
    float depth
        = ((farClip + nearClip) - 2.0 * nearClip * farClip / distanceFromCameraPlane)
        / (farClip - nearClip);
    depth = (depth + 1.0) / 2.0;
    return depth;
}

uint generateSurfaceId(ivec3 objPos) {
    uint id 
        = objPos.x 
        + objPos.y * MAX_OBJ_SCALE 
        + objPos.z  * MAX_OBJ_SCALE * MAX_OBJ_SCALE;

    /* TODO: improve this hashing, its important */

    /* Jenkins "one at time" hash function single iteration */
    id += ( id << 10u );
    id ^= ( id >>  6u );
    id += ( id <<  3u );
    id ^= ( id >> 11u );
    id += ( id << 15u );
    return id % SURFACE_LIGHT_BUFFER_COUNT;
}

vec3 calcVoxelNormal(ivec3 voxPos)
{
    vec3 normal = vec3(0, 0, 0);
    int radius = 5;
    
    for (int x = -radius + 1; x < radius; x++)
    {
        for (int y = -radius + 1; y < radius; y++)
        {
            for (int z = -radius + 1; z < radius; z++)
            {
                ivec3 samplePos = ivec3(voxPos.x + x, voxPos.y + y, voxPos.z + z);
                if (imageLoad(voxColors, samplePos).r != 0) 
                {
                    normal += vec3(x, y, z);
                }
            }
        }
    }

    return -normalize(normal);
}

void main()
{
    vec3 scale = vec3(model[0][0], model[1][1], model[2][2]);
    mat4 inverseView = inverse(view);
    vec3 camPos = vec3(inverseView[3][0], inverseView[3][1], inverseView[3][2]);
    vec3 camDir = -normalize(vec3(inverseView[2][0], inverseView[2][1], inverseView[2][2]));

    vec3 modelTranslation = vec3(model[3][0], model[3][1], model[3][2]);

    // TODO: find point this ray enters object and start traversing from there
    vec3 objPos = camPos - modelTranslation;
    ivec3 objPosInt = ivec3(
        floor(objPos.x),
        floor(objPos.y),
        floor(objPos.z));

    // TODO: find dir faster
    vec3 fragWorldPos = vec3(model * vec4(fragPos, 1));
    vec3 dir = fragWorldPos - camPos;
    dir = normalize(dir);


    uvec4 imgDat = imageLoad(voxColors, objPosInt);
    if (imgDat.x != 0) {
        writeDepth(0.0);
        writeAlbedo(vec3(1.0, 0.0, 1.0));
        writeNormal(-dir);
        writeSurfaceId(generateSurfaceId(objPosInt));
        return;
    }

    uint hit = 1;
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
    vec3 hitNormal;
    if (hit == 1) {
        hit = 0;
        while(true) {
            if(tMax.x < tMax.y) {
                if (tMax.x < tMax.z) {
                    objPosInt.x += step.x;
                    t = tMax.x;
                    if (objPosInt.x == outOfBounds.x) {
                        break;
                    }
                    tMax.x += tDelta.x;
                    hitNormal = vec3(-step.x, 0.0, 0.0);
                } else {
                    objPosInt.z += step.z;
                    t = tMax.z;
                    if (objPosInt.z == outOfBounds.z) {
                        break;
                    }
                    tMax.z += tDelta.z;
                    hitNormal = vec3(0.0, 0.0, -step.z);
                }
            } else {
                if (tMax.y < tMax.z) {
                    objPosInt.y += step.y;
                    t = tMax.y;
                    if (objPosInt.y == outOfBounds.y) {
                        break;
                    }
                    tMax.y += tDelta.y;
                    hitNormal = vec3(0.0, -step.y, 0);
                } else {
                    objPosInt.z += step.z;
                    t = tMax.z;
                    if (objPosInt.z == outOfBounds.z) {
                        break;
                    }
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


    if(hit == 0) {
        writeDepth(1);
    } else {
        //hitNormal = calcVoxelNormal(objPosInt);
        float distanceFromCameraPlane = t * dot(camDir, dir);
        writeDepth(camDistanceToDepth(distanceFromCameraPlane));
        writeAlbedo(vec3(1.0, 0.0, 0.0));
        writeNormal(hitNormal);
        writeSurfaceId(generateSurfaceId(objPosInt));
    }
}

