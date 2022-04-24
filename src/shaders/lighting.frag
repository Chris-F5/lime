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

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

vec3 depthToWorld(vec2 uv, float depth) {
    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, depth, 1.0);

    vec4 viewSpacePosition = inverse(proj) * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = inverse(view) * viewSpacePosition;

    return worldSpacePosition.xyz;
}

void main() {
    float depth = texture(samplerDepth, inUV).r;
    if (depth == 1) {
        outColor = vec4(128.0 / 255.0, 218.0 / 255.0, 251.0 / 255.0, 1.0);
        return;
    }

    vec3 albedo = texture(samplerAlbedo, inUV).rgb;
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 worldPosition = depthToWorld(inUV, depth);

    vec3 lightDir = normalize(vec3(2, -5, 1));
    float ambientFraction = 0.95;
    float normalFraction = 0.05;
    float normalLight = dot(normal, -lightDir);
    float light = ambientFraction + normalFraction * normalLight;

#if OUTPUT_POSITION == 1
    outColor = vec4(worldPosition / 40 * light, 1.0);
#else
    outColor = vec4(albedo * light, 1.0);
#endif
}
