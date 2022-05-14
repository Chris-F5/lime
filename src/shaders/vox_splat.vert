#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    mat4 view;
    mat4 proj;
    float nearClip;
    float farClip;
    uint time;
    uint movedThisFrame;
    uint screenWidth;
    uint screenHeight;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in uint color;

layout(location = 0) out vec3 outColor;

out gl_PerVertex
{
    vec4 gl_Position;
    float gl_PointSize;
};

void main ()
{
    float voxelSize = 1;
    /*
       mat4 inverseView = inverse(view);
       vec3 camPos = vec3(inverseView[3][0], inverseView[3][1], inverseView[3][2]);
       float distanceFromCamera = distance(camPos, pos);

       gl_Position = proj * view * vec4(pos, 1.0);
       gl_PointSize = (voxelSize * max(screenWidth, screenHeight)) / distanceFromCamera;
     */

    vec2 points[8];
    {
        vec4 v = proj * view * vec4(pos + vec3(0, 0, 0), 1.0);
        points[0] = v.xy / v.w;
    }
    {
        vec4 v = proj * view * vec4(pos + vec3(voxelSize, 0, 0), 1.0);
        points[1] = v.xy / v.w;
    }
    {
        vec4 v = proj * view * vec4(pos + vec3(0, voxelSize, 0), 1.0);
        points[2] = v.xy / v.w;
    }
    {
        vec4 v = proj * view * vec4(pos + vec3(voxelSize, voxelSize, 0), 1.0);
        points[3] = v.xy / v.w;
    }
    {
        vec4 v = proj * view * vec4(pos + vec3(0, 0, voxelSize), 1.0);
        points[4] = v.xy / v.w;
    }
    {
        vec4 v = proj * view * vec4(pos + vec3(voxelSize, 0, voxelSize), 1.0);
        points[5] = v.xy / v.w;
    }
    {
        vec4 v = proj * view * vec4(pos + vec3(0, voxelSize, voxelSize), 1.0);
        points[6] = v.xy / v.w;
    }
    {
        vec4 v = proj * view * vec4(pos + vec3(voxelSize, voxelSize, voxelSize), 1.0);
        points[7] = v.xy / v.w;
    }

    vec2 minCoords, maxCoords;
    for(int i = 0; i < points.length(); i++) {
        vec2 point = points[i];
        if(point.x < minCoords.x) {
            minCoords.x = point.x;
        }
        if(point.y < minCoords.y) {
            minCoords.y = point.y;
        }
        if(point.x > maxCoords.x) {
            maxCoords.x = point.x;
        }
        if(point.y > maxCoords.y) {
            maxCoords.y = point.y;
        }
    }
    vec2 diff = (maxCoords - minCoords) * vec2(screenWidth, screenHeight);

    if (diff.x > diff.y) {
        gl_PointSize = diff.x / 2.0;
    } else {
        gl_PointSize = diff.y / 2.0;
    }
    gl_Position = vec4(
        (minCoords.x + maxCoords.x) / 2.0,
        (minCoords.y + maxCoords.y) / 2.0,
        0.0,
        1.0);

    outColor = vec3(1.0, 0.0, 1.0);
}
