#ifndef LIME_GEOMETRY_PASS_H
#define LIME_GEOMETRY_PASS_H

#include <vulkan/vulkan.h>
#include <cglm/types.h>

typedef struct {
    vec3 pos;
} ObjectVertex;

void createGeometryRenderPass(
    VkDevice logicalDevice,
    VkFormat depthImageFormat,
    uint32_t colorAttachmentCount,
    VkFormat* colorAttachmentFormats,
    VkRenderPass* renderPass);

void createObjectGeometryPipeline(
    VkDevice logicalDevice,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    uint32_t subpassIndex,
    VkExtent2D presentExtent,
    VkShaderModule vertShaderModule,
    VkShaderModule fragShaderModule,
    uint32_t colorAttachmentCount,
    VkPipeline* pipeline);

#endif
