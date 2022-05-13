#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
    float nearClip;
    float farClip;
    uint time;
    uint movedThisFrame;
};

layout(set = 1, binding = 0) uniform sampler2D samplerDepth;
layout(set = 1, binding = 1) uniform sampler2D samplerAlbedo;
layout(set = 1, binding = 2) uniform sampler2D samplerNormal;
layout(set = 1, binding = 3) uniform usampler2D samplerSurfaceId;
layout(set = 1, binding = 4) buffer SurfaceLightBuffer {
    uint surfaceLightBuffer[];
};
layout(set = 1, binding = 5, rg16ui) uniform uimage2D lightAccumulateImage;

layout(set = 2, binding = 0) uniform ShadowVolumeUniformBuffer {
    ivec3 shadowVolumeSize;
};

layout(set = 2, binding = 1, r8ui) uniform uimage3D shadowVolume;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

#define SURFACE_LIGHT_MAX 8388607

vec3 lightDir = normalize(vec3(0, -1, 1));
uint samples = 4;

uint currentSeed;
uint randomByte()
{
    /* Jenkins "one at time" hash function single iteration */
    currentSeed += ( currentSeed << 10u );
    currentSeed ^= ( currentSeed >>  6u );
    currentSeed += ( currentSeed <<  3u );
    currentSeed ^= ( currentSeed >> 11u );
    currentSeed += ( currentSeed << 15u );
    return (currentSeed ^ (currentSeed >> 8)) % 256;
}

float goldenRatioMod1 = 0.61803398874989484820458683436563811772030917980576;
vec2 currentGoldenRandom;
vec2 goldenRandom()
{
    currentGoldenRandom.x = mod(currentGoldenRandom.x + goldenRatioMod1, 1.0);
    currentGoldenRandom.y = mod(currentGoldenRandom.y + goldenRatioMod1, 1.0);
    return vec2(currentGoldenRandom);
}

vec2 randomVec2()
{
    // BLUEISH NOISE
    return goldenRandom();

    // WHITE NOISE
    //return vec2(float(randomByte()) / 255.0, float(randomByte()) / 255.0);
}

vec3 cosinWeightedHemisphere()
{
    vec2 randPoint = randomVec2();

    float radial = sqrt(randPoint.x);
    float theta = 2 * 3.141592 * randPoint.y;
    float x = radial * cos(theta);
    float z = radial * sin(theta);
    return vec3(x, sqrt(1 - randPoint.x), z);
}

uint getSurfaceLight(uint surfaceId) {
    return surfaceLightBuffer[surfaceId];
}

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

mat3 makeRotMatFromDir(vec3 dir) {
    vec3 right;
    if(dir.x == 0 && dir.z == 0) {
        right = vec3(1, 0, 0);
    } else {
        right = normalize(cross(dir, vec3(0, 1, 0)));
    }
    vec3 up = normalize(cross(right, dir));

    mat3 rotMat;
    rotMat[0] = right;
    rotMat[1] = dir;
    rotMat[2] = up;
    return rotMat;
}

vec3 pickRayDir(vec3 normal) {
    vec3 hemisphereDir = cosinWeightedHemisphere();

    mat3 rotMat = makeRotMatFromDir(normal);
    return rotMat * hemisphereDir;
}

bool traceRay(vec3 rayDir, vec3 worldPos)
{
    if(worldPos.x < 0
        || worldPos.x >= shadowVolumeSize.x
        || worldPos.y < 0
        || worldPos.y >= shadowVolumeSize.y) {
        return false;
    }

    ivec3 shadowVoxPosInt = ivec3(worldPos + rayDir / 2);
    bool hit = false;

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

    /* RAY TRAVERSAL */
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

        if (sampleShadowVolume(shadowVoxPosInt) && t > 2) {
            hit = true;
            break;
        }
    }
    return hit;
}

float sampleSkyLight(vec3 rayDir) {
    //return max(0, dot(rayDir, -lightDir)) * 2;
    return abs(min(1.0, (dot(rayDir, -lightDir) + 1) / 2));
}

void main() {
    // TODO: improve rng
    currentSeed
        = uint(gl_FragCoord.x + gl_FragCoord.y * 100 + time * 10000);
    randomByte();
    currentGoldenRandom.x = randomByte() / 255.0;
    currentGoldenRandom.y = randomByte() / 255.0;

    float depth = texture(samplerDepth, inUV).r;
    if (depth == 1) {
        outColor = vec4(128.0 / 255.0, 218.0 / 255.0, 251.0 / 255.0, 1.0);
        return;
    }

    vec3 albedo = texture(samplerAlbedo, inUV).rgb;
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 worldPos = depthToWorld(inUV, depth);
    uint surfaceId = texture(samplerSurfaceId, inUV).r;

    if (normal == vec3(0, 0, 0)) {
        outColor = vec4(1.0, 0.0, 1.0, 1.0);
        return;
    }

    // TODO: world pos out of bounds of shadow volume

    float monteCarloLight = 0;
    for (int i = 0; i < samples; i++) {
        vec3 rayDir = pickRayDir(normal);
        if(!traceRay(rayDir, worldPos)) {
            monteCarloLight += sampleSkyLight(rayDir);
        }
    }
    monteCarloLight /= samples;

    uint surfaceLightInt = getSurfaceLight(surfaceId);
    float surfaceLight = float(surfaceLightInt) / float(SURFACE_LIGHT_MAX);

    uint monteCarloLightInt = int(monteCarloLight * 65535.0);
    if(movedThisFrame == 0) {
        uvec4 oldLightDat = imageLoad(lightAccumulateImage, ivec2(gl_FragCoord));
        uint oldLightInt = oldLightDat.r;
        uint staticTime = oldLightDat.g;
        float newFraction = 1.0 / float(staticTime);

        uint convergeLightInt = int(monteCarloLightInt * newFraction + oldLightInt * (1.0 - newFraction));
        monteCarloLight = convergeLightInt / 65535.0;

        uvec4 newLightDat = uvec4(convergeLightInt, staticTime + 1, 0, 0);
        imageStore(lightAccumulateImage, ivec2(gl_FragCoord), newLightDat);
    } else {
        uvec4 newLightDat = uvec4(monteCarloLightInt, 1, 0, 0);
        imageStore(lightAccumulateImage, ivec2(gl_FragCoord), newLightDat);
    }

    float ambientFraction = 0.05;
    float normalFraction  = 0.00;
    float monteCarloFraction = 0.95;
    float surfaceFraction = 0.00;
    float normalLight = dot(normal, -lightDir);
    float light
        = ambientFraction
        + normalFraction * normalLight
        + monteCarloFraction * monteCarloLight
        + surfaceFraction * surfaceLight;

    //outColor = vec4(worldPos / 200 * light, 1.0);
    outColor = vec4(light, light, light, 1.0);
    //outColor = vec4(normal, 1.0);
}
