#ifndef LIME_LIHGTING_PASS_H
#define LIME_LIHGTING_PASS_H

#include <vulkan/vulkan.h>

void createLightingRenderPass(
    VkDevice logicalDevice,
    VkFormat swapImageFormat,
    VkRenderPass* renderPass);

void createLightingPipeline(
    VkDevice logicalDevice,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    uint32_t subpassIndex,
    VkExtent2D presentExtent,
    VkShaderModule vertShaderModule,
    VkShaderModule fragShaderModule,
    VkPipeline* pipeline);

#endif
