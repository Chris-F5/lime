#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "camera.h"
#include "obj_storage.h"
#include "open-simplex-noise.h"
#include "renderer.h"
#include "utils.h"
#include "vk_device.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

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
        vec3 objPos = { 0.0f, 0.0f, 0.0f };
        ivec3 objSize = { 255, 255, 255 };
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

        uint32_t objVoxCount = objSize[0] * objSize[1] * objSize[2];
        uint8_t* voxels = malloc(objVoxCount);

        struct osn_context* noiseCtx;
        open_simplex_noise(432, &noiseCtx);
        double noiseScale = 40;

        for (uint32_t i = 0; i < objVoxCount; i++) {
            uint32_t x = i % objSize[0];
            uint32_t y = i / objSize[0] % objSize[1];
            uint32_t z = i / objSize[0] / objSize[1];
            double noiseValue = (
                open_simplex_noise3(
                    noiseCtx,
                    x / noiseScale,
                    y / noiseScale,
                    z / noiseScale) + 1
                ) / 2.0;
            if (noiseValue < 0.9 * ((float)(objSize[1] - y - 1) / (float)objSize[1])) {
                voxels[i] = 1;
            } else {
                voxels[i] = 0;
            }
        }

        ShadowVolume_splatVoxObject(
            &renderer.shadowVolume,
            vkDevice.logical,
            vkDevice.graphicsQueue,
            vkDevice.transientCommandPool,
            objSize,
            voxels);
        ObjectStorage_updateVoxColors(
            &renderer.objStorage,
            vkDevice.logical,
            vkDevice.transientCommandPool,
            vkDevice.graphicsQueue,
            objRef,
            voxels);

        free(voxels);
    }

    /*
    {
        vec3 objPos = { 20.0f, -10.0f, 20.0f };
        ivec3 objSize = { 10, 110, 10 };
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

        uint32_t objVoxCount = objSize[0] * objSize[1] * objSize[2];
        uint8_t* voxels = malloc(objVoxCount);

        for (uint32_t i = 0; i < objVoxCount; i++) {
            voxels[i] = i % 6 == 0;
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
    */

    Renderer_recreateCommandBuffers(&renderer, vkDevice.logical);

    float aspectRatio
        = (float)renderer.presentExtent.width
        / (float)renderer.presentExtent.height;
    Camera camera;
    Camera_init(&camera, aspectRatio);
    camera.pos[0] = 40;
    camera.pos[1] = 260;
    camera.pos[2] = 40;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        bool movedThisFrame = Camera_userInput(&camera, window);

        mat4 view, proj;
        Camera_viewMat(&camera, view);
        Camera_projMat(&camera, proj);

        Renderer_drawFrame(
            &renderer,
            &vkDevice,
            view,
            proj,
            camera.nearClip,
            camera.farClip,
            movedThisFrame);
    }

    vkDeviceWaitIdle(vkDevice.logical);

    Renderer_destroy(&renderer, vkDevice.logical);
    VulkanDevice_destroy(&vkDevice);

    return 0;
}
