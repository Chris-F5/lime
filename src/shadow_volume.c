#include "./shadow_volume.h"

#include <string.h>

#include "./vk_utils.h"

static void createShadowVolumeImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkExtent3D extent,
    VkFormat* format,
    VkImage* image,
    VkDeviceMemory* imageMemory,
    VkImageView* imageView)
{
    *format = VK_FORMAT_R8_UINT;

    VkImageCreateInfo imageCreateInfo;
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = NULL;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
    imageCreateInfo.format = *format;
    imageCreateInfo.extent = extent;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage
        = VK_IMAGE_USAGE_TRANSFER_DST_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = NULL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    handleVkResult(
        vkCreateImage(
            logicalDevice,
            &imageCreateInfo,
            NULL,
            image),
        "creating shadow volume image");

    allocateImageMemory(
        logicalDevice,
        physicalDevice,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        *image,
        imageMemory);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = NULL;
    imageViewCreateInfo.flags = 0;
    imageViewCreateInfo.image = *image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
    imageViewCreateInfo.format = *format;
    imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateInfo.subresourceRange = subresourceRange;
    handleVkResult(
        vkCreateImageView(
            logicalDevice,
            &imageViewCreateInfo,
            NULL,
            imageView),
        "creating shadow volume image view");
}

static void createShadowVolumeDescriptorSetLayout(
    VkDevice logicalDevice,
    VkDescriptorSetLayout* descriptorSetLayout)
{
    VkDescriptorSetLayoutBinding uboBinding;
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding imageBinding;
    imageBinding.binding = 1;
    imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageBinding.descriptorCount = 1;
    imageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    imageBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding bindings[] = {
        uboBinding,
        imageBinding
    };

    VkDescriptorSetLayoutCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    createInfo.pBindings = bindings;
    handleVkResult(
        vkCreateDescriptorSetLayout(
            logicalDevice,
            &createInfo,
            NULL,
            descriptorSetLayout),
        "creating shadow volume descriptor set layout");
}

void updateShadowVolumeDescriptorSet(
    VkDevice logicalDevice,
    VkBuffer uboBuffer,
    VkImageView imageView,
    VkImageLayout imageLayout,
    VkDescriptorSet descriptorSet)
{
    VkDescriptorBufferInfo uboBufferInfo;
    uboBufferInfo.buffer = uboBuffer;
    uboBufferInfo.offset = 0;
    uboBufferInfo.range = sizeof(ShadowVolumeUniformData);

    VkWriteDescriptorSet uboWrite;
    uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboWrite.pNext = NULL;
    uboWrite.dstSet = descriptorSet;
    uboWrite.dstBinding = 0;
    uboWrite.dstArrayElement = 0;
    uboWrite.descriptorCount = 1;
    uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboWrite.pImageInfo = NULL;
    uboWrite.pBufferInfo = &uboBufferInfo;
    uboWrite.pTexelBufferView = NULL;

    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = VK_NULL_HANDLE;
    imageInfo.imageView = imageView;
    imageInfo.imageLayout = imageLayout;

    VkWriteDescriptorSet imageWrite;
    imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    imageWrite.pNext = NULL;
    imageWrite.dstSet = descriptorSet;
    imageWrite.dstBinding = 1;
    imageWrite.dstArrayElement = 0;
    imageWrite.descriptorCount = 1;
    imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWrite.pImageInfo = &imageInfo;
    imageWrite.pBufferInfo = NULL;
    imageWrite.pTexelBufferView = NULL;

    VkWriteDescriptorSet writes[] = {
        uboWrite,
        imageWrite,
    };

    vkUpdateDescriptorSets(
        logicalDevice,
        sizeof(writes) / sizeof(writes[0]),
        writes,
        0,
        NULL);
}

void ShadowVolume_init(
    ShadowVolume* sv,
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkDescriptorPool descriptorPool,
    VkQueue graphicsQueue,
    VkCommandPool transientGraphicsCommandPool,
    ivec3 size)
{
    sv->size[0] = size[0];
    sv->size[1] = size[1];
    sv->size[2] = size[2];
    sv->extent = (VkExtent3D) { (size[0] + 1) / 2, (size[1] + 1) / 2, (size[2]) / 2 };
    sv->texelCount = sv->extent.width * sv->extent.height * sv->extent.depth;

    createBuffer(
        logicalDevice,
        physicalDevice,
        sizeof(ShadowVolumeUniformData),
        0,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &sv->uniformBuffer,
        &sv->uniformBufferMemory);

    ShadowVolumeUniformData* uniformDat;
    handleVkResult(
        vkMapMemory(
            logicalDevice,
            sv->uniformBufferMemory,
            0,
            sizeof(ShadowVolumeUniformData),
            0,
            (void**)&uniformDat),
        "mapping shadow volume uniform buffer memory");
    uniformDat->size[0] = sv->size[0];
    uniformDat->size[1] = sv->size[1];
    uniformDat->size[2] = sv->size[2];
    vkUnmapMemory(logicalDevice, sv->uniformBufferMemory);

    createBuffer(
        logicalDevice,
        physicalDevice,
        sv->texelCount,
        0,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &sv->stagingBuffer,
        &sv->stagingBufferMemory);

    uint8_t* staging;
    handleVkResult(
        vkMapMemory(
            logicalDevice,
            sv->stagingBufferMemory,
            0,
            sv->texelCount,
            0,
            (void**)&staging),
        "mapping shadow volume staging buffer memory");
    memset(staging, 0, sv->texelCount);
    vkUnmapMemory(logicalDevice, sv->stagingBufferMemory);

    createShadowVolumeImage(
        logicalDevice,
        physicalDevice,
        sv->extent,
        &sv->imageFormat,
        &sv->image,
        &sv->imageMemory,
        &sv->imageView);

    VkImageSubresourceRange imageSubresourceRange;
    imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageSubresourceRange.baseMipLevel = 0;
    imageSubresourceRange.levelCount = 1;
    imageSubresourceRange.baseArrayLayer = 0;
    imageSubresourceRange.layerCount = 1;
    transitionImageLayout(
        logicalDevice,
        graphicsQueue,
        transientGraphicsCommandPool,
        sv->image,
        imageSubresourceRange,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        0,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    createShadowVolumeDescriptorSetLayout(
        logicalDevice,
        &sv->descriptorSetLayout);
    allocateDescriptorSets(
        logicalDevice,
        sv->descriptorSetLayout,
        descriptorPool,
        1,
        &sv->descriptorSet);
    updateShadowVolumeDescriptorSet(
        logicalDevice,
        sv->uniformBuffer,
        sv->imageView,
        VK_IMAGE_LAYOUT_GENERAL,
        sv->descriptorSet);

    copyBufferToGeneralColorImage(
        logicalDevice,
        transientGraphicsCommandPool,
        graphicsQueue,
        sv->stagingBuffer,
        sv->image,
        sv->extent);
}

void ShadowVolume_splatVoxObject(
    ShadowVolume* sv,
    VkDevice logicalDevice,
    VkQueue graphicsQueue,
    VkCommandPool transientGraphicsCommandPool,
    ivec3 objSize,
    uint8_t* voxelColors)
{
    uint32_t objVoxCount = objSize[0] * objSize[1] * objSize[2];

    uint8_t* staging;
    handleVkResult(
        vkMapMemory(
            logicalDevice,
            sv->stagingBufferMemory,
            0,
            sv->texelCount,
            0,
            (void**)&staging),
        "mapping shadow volume staging buffer memory");

    for (uint32_t v = 0; v < objVoxCount; v++)
        if (voxelColors[v] != 0) {
            uint32_t x = v % objSize[0];
            uint32_t y = v / objSize[0] % objSize[1];
            uint32_t z = v / objSize[0] / objSize[1];
            uint32_t byteIndex
                = (x / 2)
                + (y / 2) * sv->extent.width
                + (z / 2) * sv->extent.width * sv->extent.height;
            uint32_t bitIndex
                = (x % 2)
                + (y % 2) * 2
                + (z % 2) * 4;
            staging[byteIndex] |= (1 << bitIndex);
        }
    //memset(staging, ~0, sv->texelCount);

    vkUnmapMemory(logicalDevice, sv->stagingBufferMemory);

    // TODO: only copy region I just updated
    copyBufferToGeneralColorImage(
        logicalDevice,
        transientGraphicsCommandPool,
        graphicsQueue,
        sv->stagingBuffer,
        sv->image,
        sv->extent);
}

void ShadowVolume_destroy(
    ShadowVolume* sv,
    VkDevice logicalDevice)
{
    vkDestroyBuffer(logicalDevice, sv->uniformBuffer, NULL);
    vkFreeMemory(logicalDevice, sv->uniformBufferMemory, NULL);

    vkDestroyBuffer(logicalDevice, sv->stagingBuffer, NULL);
    vkFreeMemory(logicalDevice, sv->stagingBufferMemory, NULL);

    vkDestroyImageView(logicalDevice, sv->imageView, NULL);
    vkDestroyImage(logicalDevice, sv->image, NULL);
    vkFreeMemory(logicalDevice, sv->imageMemory, NULL);

    vkDestroyDescriptorSetLayout(logicalDevice, sv->descriptorSetLayout, NULL);
}
