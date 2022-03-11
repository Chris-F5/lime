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

void createImage(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkImageType imageType,
    VkFormat format,
    VkExtent3D extent,
    VkImageCreateFlags flags,
    VkImageUsageFlags usageFlags,
    uint32_t mipLevels,
    uint32_t arrayLayers,
    VkSampleCountFlagBits samples,
    VkImageTiling tiling,
    bool preinitialized,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkImage* image,
    VkDeviceMemory* imageMemory);

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
