#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
    if (fragColor.g > 0.5) {
        gl_FragDepth = gl_FragCoord.z;
    } else {
        gl_FragDepth = gl_FragCoord.z;
    }
    //gl_FragDepth gl_FragCoord.z
}
