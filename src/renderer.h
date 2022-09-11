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
    uint32_t time;
    uint32_t movedThisFrame;
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

    VkFormat surfaceHashImageFormat;
    VkImage surfaceHashImage;
    VkDeviceMemory surfaceHashImageMemory;
    VkImageView surfaceHashImageView;

    /* IMAGES */
    VkFormat irradianceImageFormat;
    VkImage irradianceImage;
    VkDeviceMemory irradianceImageMemory;
    VkImageView irradianceImageView;

    /* SAMPLERS */
    VkSampler gbufferSampler;
    VkSampler irradianceSampler;

    /* STORAGE IMAGES */
    VkImage lightAccumulateImage;
    VkDeviceMemory lightAccumulateImageMemory;
    VkImageView lightAccumulateImageView;

    /* UNIFORM BUFFERS */
    VkBuffer cameraUniformBuffer;
    VkDeviceMemory cameraUniformBufferMemory;

    /* DESCRIPTOR SETS */
    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout cameraDescriptorSetLayout;
    VkDescriptorSet* cameraDescriptorSets;

    VkDescriptorSetLayout gbufferDescriptorSetLayout;
    VkDescriptorSet gbufferDescriptorSet;

    VkDescriptorSetLayout irradianceDescriptorSetLayout;
    VkDescriptorSet irradianceDescriptorSet;

    /* PIPELINES */
    VkPipelineLayout objGeometryPipelineLayout;
    VkPipeline objGeometryPipeline;

    VkPipelineLayout lightingPipelineLayout;
    VkPipeline lightingPipeline;

    VkPipelineLayout denoisePipelineLayout;
    VkPipeline denoisePipeline;

    /* RENDER PASSES */
    VkRenderPass geometryRenderPass;
    VkRenderPass lightingRenderPass;
    VkRenderPass denoiseRenderPass;

    /* FRAMEBUFFERS */
    VkFramebuffer geometryFramebuffer;
    VkFramebuffer irradianceFramebuffer;
    VkFramebuffer* swapImageFramebuffers;

    /* COMMAND BUFFERS */
    VkCommandBuffer* commandBuffers;

    /* SYNCHRONIZATION */
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence renderFinishedFences[MAX_FRAMES_IN_FLIGHT];

    VkFence** swapchainImageFences;
    int currentFrame;

    uint32_t time;
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
    float farClip,
    bool movedThisFrame);

void Renderer_destroy(
    Renderer* renderer,
    VkDevice logicalDevice);

#endif
