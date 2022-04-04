#ifndef LIME_RENDERER_H
#define LIME_RENDERER_H

#include <vulkan/vulkan.h>
#include <cglm/types.h>

#include "obj_storage.h"
#include "vk_device.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct {
    mat4 view;
    mat4 proj;
} RenderDescriptorData;

typedef struct {
    /* SCENE DATA */
    ObjectStorage objStorage;

    /* GBUFFER */
    VkExtent2D presentExtent;
    VkFormat swapImageFormat;
    uint32_t swapLen;
    VkSwapchainKHR swapchain;
    VkImage* swapImages;
    VkImageView* swapImageViews;

    VkFormat depthImageFormat;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    /* DESCRIPTOR SETS */
    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout renderDescriptorSetLayout;
    VkDescriptorSet* renderDescriptorSets;

    VkBuffer renderDescriptorBuffer;
    VkDeviceMemory renderDescriptorBufferMemory;

    /* PIPELINES */
    VkRenderPass objRenderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    /* FRAMEBUFFERS */
    VkFramebuffer* framebuffers;

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
    mat4 proj);

void Renderer_destroy(
    Renderer* renderer,
    VkDevice logicalDevice);

#endif
