#ifndef LIME_SHADOW_VOLUME_H

#include <vulkan/vulkan.h>
#include <cglm/types.h>

typedef struct {
    ivec3 size;
} ShadowVolumeUniformData;

typedef struct {
    ivec3 size;
    VkExtent3D extent;
    uint32_t texelCount;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkFormat imageFormat;
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
} ShadowVolume;


void ShadowVolume_init(
    ShadowVolume* sv,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkDescriptorPool descriptorPool,
    VkQueue graphicsQueue,
    VkCommandPool transientGraphicsCommandPool,
    ivec3 size);

void ShadowVolume_splatVoxObject(
    ShadowVolume* sv,
    VkDevice logicalDevice,
    VkQueue graphicsQueue,
    VkCommandPool transientGraphicsCommandPool,
    ivec3 objSize,
    uint8_t* voxelColors);

void ShadowVolume_destroy(
    ShadowVolume* sv,
    VkDevice logicalDevice);

#endif
