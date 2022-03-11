#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/types.h>

#include <GLFW/glfw3.h>

typedef struct {
    vec3 pos;
    float yaw;
    float pitch;
    float fov;
    float aspectRatio;
    float nearClip;
    float farClip;
} Camera;

void Camera_init(Camera* camera, float aspectRatio);

void Camera_forward(const Camera* camera, vec3 forward);

void Camera_right(const Camera* camera, vec3 right);

void Camera_userInput(Camera* camera, GLFWwindow* window);

void Camera_viewMat(Camera* camera, mat4 view);

void Camera_projMat(Camera* camera, mat4 proj);

#endif
