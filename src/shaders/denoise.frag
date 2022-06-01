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
layout(set = 1, binding = 3) uniform usampler2D samplerSurfaceHash;

layout(set = 2, binding = 0) uniform sampler2D samplerIrradiance;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

float filterDistribution[] = {
    0.1, 0.1, 0.1, 0.1, 0.2, 0.1, 0.1, 0.1, 0.1
};

float getIrradiance() {
    ivec2 screenPixelCoord = ivec2(gl_FragCoord);
    uint surface = texelFetch(samplerSurfaceHash, screenPixelCoord, 0).r;
    float depth = texelFetch(samplerDepth, screenPixelCoord, 0).r;

    uint radius = (filterDistribution.length() - 1) / 2;
    float irradiance = 0;
    float missed = 0;
    for(int yo = 0; yo < filterDistribution.length(); yo++) {
        for(int xo = 0; xo < filterDistribution.length(); xo++) {
            ivec2 pos = screenPixelCoord + ivec2(xo - radius, yo - radius);
            //if(texelFetch(samplerSurfaceHash, pos, 0).r != surface) {
            if(abs(texelFetch(samplerDepth, pos, 0).r - depth) > 0.00001) {
                missed += filterDistribution[yo] * filterDistribution[xo];
                continue;
            }
            irradiance += texelFetch(samplerIrradiance, pos, 0).r 
                * filterDistribution[yo] * filterDistribution[xo];
        }
    }
    irradiance *= 1 / (1-missed);
    return irradiance;
}

void main() {
    float irradiance = getIrradiance();
    outColor = vec4(irradiance);
    //outColor = vec4(texelFetch(samplerIrradiance, ivec2(gl_FragCoord), 0));
}
