#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "camera.h"
#include "obj_storage.h"
#include "renderer.h"
#include "utils.h"
#include "vk_device.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void glfwErrorCallback(int _, const char* errorString)
{
    printf("Exiting because of GLFW error: '%s'\n", errorString);
    exit(1);
}

static VkExtent2D choosePresentExtent(
    GLFWwindow* window,
    VkSurfaceCapabilitiesKHR surfaceCapabilities)
{
    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        return surfaceCapabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D extent;

        extent.width = MIN(
            surfaceCapabilities.maxImageExtent.width,
            MAX(
                width,
                surfaceCapabilities.maxImageExtent.width));
        extent.height = MIN(
            surfaceCapabilities.maxImageExtent.height,
            MAX(
                height,
                surfaceCapabilities.maxImageExtent.height));

        return extent;
    }
}

int main()
{
    glfwSetErrorCallback(glfwErrorCallback);
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Vulkan App",
        NULL,
        NULL);

    VulkanDevice vkDevice;
    VulkanDevice_init(&vkDevice, window);

    VkExtent2D presentExtent = choosePresentExtent(
        window,
        vkDevice.physicalProperties.surfaceCapabilities);


    Renderer renderer;
    Renderer_init(
        &renderer,
        &vkDevice,
        presentExtent);

    {
        vec3 objPos = { 1.0f, 1.0f, 10.0f };
        ivec3 objSize = { 50, 30, 40 };
        ObjRef objRef;
        ObjectStorage_addObjects(
            &renderer.objStorage,
            vkDevice.logical,
            vkDevice.physical,
            vkDevice.graphicsQueue,
            vkDevice.transientCommandPool,
            1,
            &objPos,
            &objSize,
            &objRef);

        uint8_t* voxels = malloc(50 * 30 * 40);
        for (uint32_t i = 0; i < 50 * 30 * 40; i++) {
            voxels[i] = i % 7 == 0;
        }
        ObjectStorage_updateVoxColors(
            &renderer.objStorage,
            vkDevice.logical,
            vkDevice.transientCommandPool,
            vkDevice.graphicsQueue,
            objRef,
            voxels);
        free(voxels);
    }
    {
        vec3 objPos = { 4.0f, 3.0f, 8.0f };
        ivec3 objSize = { 3, 2, 1 };
        ObjRef objRef;
        ObjectStorage_addObjects(
            &renderer.objStorage,
            vkDevice.logical,
            vkDevice.physical,
            vkDevice.graphicsQueue,
            vkDevice.transientCommandPool,
            1,
            &objPos,
            &objSize,
            &objRef);
    }


    Renderer_recreateCommandBuffers(&renderer, vkDevice.logical);

    float aspectRatio
        = (float)renderer.presentExtent.width
        / (float)renderer.presentExtent.height;
    Camera camera;
    Camera_init(&camera, aspectRatio);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Camera_userInput(&camera, window);

        mat4 view, proj;
        Camera_viewMat(&camera, view);
        Camera_projMat(&camera, proj);

        Renderer_drawFrame(
            &renderer,
            &vkDevice,
            view,
            proj,
            camera.nearClip,
            camera.farClip);
    }

    vkDeviceWaitIdle(vkDevice.logical);

    Renderer_destroy(&renderer, vkDevice.logical);
    VulkanDevice_destroy(&vkDevice);

    return 0;
}
