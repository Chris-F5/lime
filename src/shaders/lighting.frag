#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
    float nearClip;
    float farClip;
};

layout(set = 1, binding = 0) uniform sampler2D samplerDepth;
layout(set = 1, binding = 1) uniform sampler2D samplerAlbedo;
layout(set = 1, binding = 2) uniform sampler2D samplerNormal;

layout(set = 2, binding = 0) uniform ShadowVolumeUniformBuffer {
    ivec3 shadowVolumeSize;
};

layout(set = 2, binding = 1, r8ui) uniform uimage3D shadowVolume;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

vec3 depthToWorld(vec2 uv, float depth) {
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, depth, 1.0);

    vec4 viewSpacePosition = inverse(proj) * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = inverse(view) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

bool sampleShadowVolume(ivec3 pos) {
    ivec3 texel = pos / 2;
    uint bitIndex
        = (pos.x % 2)
        + (pos.y % 2) * 2
        + (pos.z % 2) * 4;
    uvec4 texDat = imageLoad(shadowVolume, texel);
    //return texDat.x > 0;
    return (texDat.x & (1 << bitIndex)) > 0;
}

vec3 lightDir = normalize(vec3(0.2, -1, 0.1));

vec3 pickRayDir(vec3 normal) {
    if (dot(normal, -lightDir) > 0) {
        return -lightDir;
    } else {
        return -lightDir;
    }
}

void main() {
    float depth = texture(samplerDepth, inUV).r;
    if (depth == 1) {
        outColor = vec4(128.0 / 255.0, 218.0 / 255.0, 251.0 / 255.0, 1.0);
        return;
    }

    vec3 albedo = texture(samplerAlbedo, inUV).rgb;
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 worldPos = depthToWorld(inUV, depth);

    vec3 rayDir = pickRayDir(normal);
    ivec3 shadowVoxPosInt = ivec3(worldPos + rayDir / 2);

    // TODO: if texel pos out of bounds

    float brdf = max(0, dot(normal, rayDir));

    bool hit = false;
    /* RAY */
    if (brdf != 0) {

        /* RAY TRAVERSAL INIT */
        ivec3 step;
        ivec3 outOfBounds;
        vec3 tDelta;
        vec3 tMax;
        if (rayDir.x > 0) {
            step.x = 1;
            outOfBounds.x = int(shadowVolumeSize.x);
            tDelta.x = 1 / rayDir.x;
            tMax.x = tDelta.x * (shadowVoxPosInt.x + 1 - worldPos.x);
        } else if (rayDir.x < 0){
            step.x = -1;
            outOfBounds.x = -1;
            tDelta.x = 1 / -rayDir.x;
            tMax.x = tDelta.x * (worldPos.x - shadowVoxPosInt.x);
        } else {
            step.x = 0;
            outOfBounds.x = -1;
            tDelta.x = 0.0;
            tMax.x = 1.0 / 0.0; // infinity
        }

        if (rayDir.y > 0) {
            step.y = 1;
            outOfBounds.y = int(shadowVolumeSize.y);
            tDelta.y = 1 / rayDir.y;
            tMax.y = tDelta.y * (shadowVoxPosInt.y + 1 - worldPos.y);
        } else if (rayDir.y < 0){
            step.y = -1;
            outOfBounds.y = -1;
            tDelta.y = 1 / -rayDir.y;
            tMax.y = tDelta.y * (worldPos.y - shadowVoxPosInt.y);
        } else {
            step.y = 0;
            outOfBounds.y = -1;
            tDelta.y = 0.0;
            tMax.y = 1.0 / 0.0; // infinity
        }

        if (rayDir.z > 0) {
            step.z = 1;
            outOfBounds.z = int(shadowVolumeSize.z);
            tDelta.z = 1 / rayDir.z;
            tMax.z = tDelta.z * (shadowVoxPosInt.z + 1 - worldPos.z);
        } else if(rayDir.z < 0) {
            step.z = -1;
            outOfBounds.z = -1;
            tDelta.z = 1 / -rayDir.z;
            tMax.z = tDelta.z * (worldPos.z - shadowVoxPosInt.z);
        } else {
            step.z = 0;
            outOfBounds.z = -1;
            tDelta.z = 0.0;
            tMax.z = 1.0 / 0.0; // infinity
        }

        float t = 0;
        while(true) {
            if(tMax.x < tMax.y) {
                if (tMax.x < tMax.z) {
                    shadowVoxPosInt.x += step.x;
                    t = tMax.x;
                    if (shadowVoxPosInt.x == outOfBounds.x) {
                        break;
                    }
                    tMax.x += tDelta.x;
                } else {
                    shadowVoxPosInt.z += step.z;
                    t = tMax.z;
                    if (shadowVoxPosInt.z == outOfBounds.z) {
                        break;
                    }
                    tMax.z += tDelta.z;
                }
            } else {
                if (tMax.y < tMax.z) {
                    shadowVoxPosInt.y += step.y;
                    t = tMax.y;
                    if (shadowVoxPosInt.y == outOfBounds.y) {
                        break;
                    }
                    tMax.y += tDelta.y;
                } else {
                    shadowVoxPosInt.z += step.z;
                    t = tMax.z;
                    if (shadowVoxPosInt.z == outOfBounds.z) {
                        break;
                    }
                    tMax.z += tDelta.z;
                }
            }

            if (sampleShadowVolume(shadowVoxPosInt)) {
                hit = true;
                break;
            }
        }
    }

    float ambientFraction = 0.45;
    float normalFraction  = 0.05;
    float directFraction  = 0.50;
    float normalLight = dot(normal, -lightDir);
    float directLight = hit ? 0 : brdf;
    float light 
        = ambientFraction 
        + normalFraction * normalLight
        + directFraction * directLight;

    outColor = vec4(worldPos / 200 * light, 1.0);
}
