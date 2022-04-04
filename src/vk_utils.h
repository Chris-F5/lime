#ifndef LIME_VK_UTILS_H
#define LIME_VK_UTILS_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

void handleVkResult(VkResult result, char* message);

uint32_t findPhysicalDeviceMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t memoryTypeBits,
    VkMemoryPropertyFlags properties);

void createBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferCreateFlags flags,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkBuffer* buffer,
    VkDeviceMemory* bufferMemory);

void allocateImageMemory(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkImage image,
    VkDeviceMemory* imageMemory);

void transitionImageLayout(
    VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    VkImage image,
    VkImageSubresourceRange imageSubresourceRange,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags dstStageMask);

void createShaderModule(
    VkDevice device,
    const char* srcFileName,
    VkShaderModule* shaderModule);

void allocateDescriptorSets(
    VkDevice device,
    VkDescriptorSetLayout layout,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    VkDescriptorSet* descriptorSets);

#endif
