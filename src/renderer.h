#ifndef LIME_RENDERER_H
#define LIME_RENDERER_H

#include <vulkan/vulkan.h>
#include <cglm/types.h>

#include "./vk_device.h"
#include "./obj_storage.h"
#include "./shadow_volume.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct {
    mat4 view;
    mat4 proj;
    float nearClip; // TODO : is there a way to guarantee 32 bit
    float farClip;
} CameraUniformData;

typedef struct {
    /* SCENE DATA */
    ObjectStorage objStorage;
    ShadowVolume shadowVolume;

    /* SWAPCHAIN */
    VkExtent2D presentExtent;
    VkFormat swapImageFormat;
    uint32_t swapLen;
    VkSwapchainKHR swapchain;
    VkImage* swapImages;
    VkImageView* swapImageViews;

    /* GBUFFER */
    VkFormat depthImageFormat;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkFormat albedoImageFormat;
    VkImage albedoImage;
    VkDeviceMemory albedoImageMemory;
    VkImageView albedoImageView;

    VkFormat normalImageFormat;
    VkImage normalImage;
    VkDeviceMemory normalImageMemory;
    VkImageView normalImageView;

    VkFormat surfaceIdImageFormat;
    VkImage surfaceIdImage;
    VkDeviceMemory surfaceIdImageMemory;
    VkImageView surfaceIdImageView;

    /* SAMPLERS */
    VkSampler gbufferSampler;

    /* STORAGE BUFFERS */
    VkBuffer surfaceLightBuffer;
    VkDeviceMemory surfaceLightBufferMemory;

    /* UNIFORM BUFFERS */
    VkBuffer cameraUniformBuffer;
    VkDeviceMemory cameraUniformBufferMemory;

    /* DESCRIPTOR SETS */
    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout cameraDescriptorSetLayout;
    VkDescriptorSet* cameraDescriptorSets;

    VkDescriptorSetLayout lightingPassDescriptorSetLayout;
    VkDescriptorSet lightingPassDescriptorSet;

    /* PIPELINES */
    VkPipelineLayout objGeometryPipelineLayout;
    VkPipeline objGeometryPipeline;

    VkPipelineLayout lightingPipelineLayout;
    VkPipeline lightingPipeline;

    /* RENDER PASSES */
    VkRenderPass geometryRenderPass;
    VkRenderPass lightingRenderPass;

    /* FRAMEBUFFERS */
    VkFramebuffer geometryFramebuffer;
    VkFramebuffer* swapImageFramebuffers;

    /* COMMAND BUFFERS */
    VkCommandBuffer* commandBuffers;

    /* SYNCHRONIZATION */
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence renderFinishedFences[MAX_FRAMES_IN_FLIGHT];

    VkFence** swapchainImageFences;
    int currentFrame;
} Renderer;

void Renderer_init(
    Renderer* renderer,
    VulkanDevice* device,
    VkExtent2D presentExtent);

void Renderer_recreateCommandBuffers(
    Renderer* renderer,
    VkDevice logicalDevice);

void Renderer_drawFrame(
    Renderer* renderer,
    VulkanDevice* device,
    mat4 view,
    mat4 proj,
    float nearClip,
    float farClip);

void Renderer_destroy(
    Renderer* renderer,
    VkDevice logicalDevice);

#endif
