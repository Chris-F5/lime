#include "./renderer.h"

#include <stdlib.h>
#include <string.h>

#include <cglm/cglm.h>

#include "utils.h"
#include "vk_utils.h"

#include "./geometry_pass.h"
#include "./lighting_pass.h"
#include "./screen_images.h"

/*
 * surface hash irradiance cache element:
 *   8 bit  heat
 *   24 bit sample count
 *   32 bit sample sum
 *   32 bit scaled sample squared sum
 * = 96 bits = 12 bytes
 */
#define SURFACE_HASH_IRRADIANCE_CACHE_ELEMENT_COUNT 1000000
#define SURFACE_HASH_IRRADIANCE_CACHE_ELEMENT_SIZE 12

#define OBJ_VERT_COUNT 36

static void createGeometryFramebuffer(
    VkDevice logicalDevice,
    VkRenderPass renderPass,
    VkExtent2D extent,
    uint32_t attachmentCount,
    VkImageView* attachments,
    VkFramebuffer* framebuffer)
{
    VkFramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext = NULL;
    framebufferCreateInfo.flags = 0;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = attachmentCount;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = extent.width;
    framebufferCreateInfo.height = extent.height;
    framebufferCreateInfo.layers = 1;

    handleVkResult(
        vkCreateFramebuffer(
            logicalDevice,
            &framebufferCreateInfo,
            NULL,
            framebuffer),
        "creating graphics framebuffer");
}

static void createSwapImageFramebuffers(
    VkDevice logicalDevice,
    VkRenderPass renderPass,
    VkExtent2D extent,
    uint32_t count,
    VkImageView* swapImageViews,
    VkFramebuffer* framebuffers)
{
    for (uint32_t i = 0; i < count; i++) {
        VkImageView attachments[] = {
            swapImageViews[i],
        };

        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = NULL;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount
            = sizeof(attachments) / sizeof(attachments[0]);
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = extent.width;
        framebufferCreateInfo.height = extent.height;
        framebufferCreateInfo.layers = 1;

        handleVkResult(
            vkCreateFramebuffer(
                logicalDevice,
                &framebufferCreateInfo,
                NULL,
                &framebuffers[i]),
            "creating swap image framebuffer");
    }
}

void createGbufferDescriptorSetLayout(
    VkDevice logicalDevice,
    uint32_t imageCount,
    VkDescriptorSetLayout* descriptorSetLayout)
{
    VkDescriptorSetLayoutBinding* bindings = (VkDescriptorSetLayoutBinding*)
        malloc(imageCount * sizeof(VkDescriptorSetLayoutBinding));
    for (uint32_t i = 0; i < imageCount; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[i].pImmutableSamplers = NULL;
    }

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.sType
        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.pNext = NULL;
    layoutCreateInfo.flags = 0;
    layoutCreateInfo.bindingCount = imageCount;
    layoutCreateInfo.pBindings = bindings;

    handleVkResult(
        vkCreateDescriptorSetLayout(
            logicalDevice,
            &layoutCreateInfo,
            NULL,
            descriptorSetLayout),
        "creating gbuffer descriptor set layout");
    free(bindings);
}

void updateGbufferDescriptorSetBindings(
    VkDevice logicalDevice,
    VkSampler sampler,
    uint32_t imageCount,
    VkImageView* imageViews,
    VkDescriptorSet dstSet)
{
    VkDescriptorImageInfo* imageInfos = (VkDescriptorImageInfo*)malloc(
        imageCount * sizeof(VkDescriptorImageInfo));
    for (uint32_t i = 0; i < imageCount; i++) {
        imageInfos[i].sampler = sampler;
        imageInfos[i].imageView = imageViews[i];
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = NULL;
    write.dstSet = dstSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = imageCount;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = imageInfos;
    write.pBufferInfo = NULL;
    write.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(logicalDevice, 1, &write, 0, NULL);

    free(imageInfos);
}

static void recordRenderCommandBuffer(
    VkDevice logicalDevice,
    VkExtent2D presentExtent,
    VkRenderPass geometryRenderPass,
    VkRenderPass lightingRenderPass,
    VkFramebuffer geometryFramebuffer,
    VkFramebuffer lightingFramebuffer,
    uint32_t geometryPassClearValueCount,
    VkClearValue* geometryPassClearValues,
    uint32_t lightingPassClearValueCount,
    VkClearValue* lightingPassClearValues,
    VkPipeline objGeometryPipeline,
    VkPipeline lightingPipeline,
    VkPipelineLayout objGeometryPipelineLayout,
    VkPipelineLayout lightingPipelineLayout,
    uint32_t objGeometryDescriptorSetCount,
    VkDescriptorSet* objGeometryDescriptorSets,
    uint32_t lightingDescriptorSetCount,
    VkDescriptorSet* lightingDescriptorSets,
    uint32_t voxObjCount,
    VkDescriptorSet* voxObjDescriptorSets,
    VkCommandBuffer commandBuffer)
{
    /* BEGIN COMMAND BUFFER */
    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = NULL;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = NULL;
    handleVkResult(
        vkBeginCommandBuffer(commandBuffer, &beginInfo),
        "begin recording render command buffers");

    /* GEOMETRY PASS */
    {
        VkRenderPassBeginInfo geometryPassInfo;
        geometryPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        geometryPassInfo.pNext = NULL;
        geometryPassInfo.renderPass = geometryRenderPass;
        geometryPassInfo.framebuffer = geometryFramebuffer;
        geometryPassInfo.renderArea.offset = (VkOffset2D) { 0, 0 };
        geometryPassInfo.renderArea.extent = presentExtent;
        geometryPassInfo.clearValueCount = geometryPassClearValueCount;
        geometryPassInfo.pClearValues = geometryPassClearValues;

        vkCmdBeginRenderPass(
            commandBuffer,
            &geometryPassInfo,
            VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        objGeometryPipeline);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        objGeometryPipelineLayout,
        0,
        objGeometryDescriptorSetCount,
        objGeometryDescriptorSets,
        0,
        NULL);

    for (int o = 0; o < voxObjCount; o++) {
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            objGeometryPipelineLayout,
            objGeometryDescriptorSetCount,
            1,
            &voxObjDescriptorSets[o],
            0,
            NULL);
        vkCmdDraw(
            commandBuffer,
            OBJ_VERT_COUNT,
            1,
            0,
            0);
    }
    vkCmdEndRenderPass(commandBuffer);

    /* GRAPHICS -> LIGHTING : MEMORY BARRIERS */
    VkMemoryBarrier memoryBarrier;
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.pNext = NULL;
    memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        1,
        &memoryBarrier,
        0,
        NULL,
        0,
        NULL);

    /* LIGHTING PASS */
    {
        VkRenderPassBeginInfo renderPassInfo;
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.pNext = NULL;
        renderPassInfo.renderPass = lightingRenderPass;
        renderPassInfo.framebuffer = lightingFramebuffer;
        renderPassInfo.renderArea.offset = (VkOffset2D) { 0, 0 };
        renderPassInfo.renderArea.extent = presentExtent;
        renderPassInfo.clearValueCount = lightingPassClearValueCount;
        renderPassInfo.pClearValues = lightingPassClearValues;

        vkCmdBeginRenderPass(
            commandBuffer,
            &renderPassInfo,
            VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            lightingPipeline);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            lightingPipelineLayout,
            0,
            lightingDescriptorSetCount,
            lightingDescriptorSets,
            0,
            NULL);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
    }

    /* END COMAND BUFFER */
    handleVkResult(
        vkEndCommandBuffer(commandBuffer),
        "recording render command buffer");
}

void Renderer_init(
    Renderer* renderer,
    VulkanDevice* device,
    VkExtent2D presentExtent)
{
    ObjectStorage_init(
        &renderer->objStorage,
        device->logical,
        device->physical);

    /* SWAPCHAIN */
    renderer->presentExtent = presentExtent;
    renderer->swapImageFormat = device->physicalProperties.surfaceFormat.format;
    createSwapchain(
        device->logical,
        device->surface,
        device->physicalProperties.surfaceFormat,
        device->physicalProperties.surfaceCapabilities,
        device->physicalProperties.presentMode,
        device->physicalProperties.graphicsFamilyIndex,
        device->physicalProperties.presentFamilyIndex,
        renderer->presentExtent,
        &renderer->swapchain);
    handleVkResult(
        vkGetSwapchainImagesKHR(
            device->logical,
            renderer->swapchain,
            &renderer->swapLen,
            NULL),
        "getting swapchain image count");
    renderer->swapImages
        = (VkImage*)malloc(renderer->swapLen * sizeof(VkImage));
    handleVkResult(
        vkGetSwapchainImagesKHR(
            device->logical,
            renderer->swapchain,
            &renderer->swapLen,
            renderer->swapImages),
        "getting swapchain images");
    renderer->swapImageViews
        = (VkImageView*)malloc(renderer->swapLen * sizeof(VkImageView));
    for (int i = 0; i < renderer->swapLen; i++) {
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
        imageViewCreateInfo.image = renderer->swapImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = renderer->swapImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange = subresourceRange;
        handleVkResult(
            vkCreateImageView(
                device->logical,
                &imageViewCreateInfo,
                NULL,
                &renderer->swapImageViews[i]),
            "creating swapchain image view");
    }

    /* GBUFFER */
    renderer->depthImageFormat = device->physicalProperties.depthImageFormat;
    createDepthImage(
        device->logical,
        device->physical,
        renderer->depthImageFormat,
        renderer->presentExtent,
        &renderer->depthImage,
        &renderer->depthImageMemory,
        &renderer->depthImageView);

    renderer->albedoImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    createColorAttachmentImage(
        device->logical,
        device->physical,
        renderer->albedoImageFormat,
        renderer->presentExtent,
        &renderer->albedoImage,
        &renderer->albedoImageMemory,
        &renderer->albedoImageView);

    renderer->normalImageFormat = VK_FORMAT_B8G8R8A8_SNORM;
    createColorAttachmentImage(
        device->logical,
        device->physical,
        renderer->normalImageFormat,
        renderer->presentExtent,
        &renderer->normalImage,
        &renderer->normalImageMemory,
        &renderer->normalImageView);

    renderer->surfaceHashImageFormat = VK_FORMAT_R32_UINT;
    createColorAttachmentImage(
        device->logical,
        device->physical,
        renderer->surfaceHashImageFormat,
        renderer->presentExtent,
        &renderer->surfaceHashImage,
        &renderer->surfaceHashImageMemory,
        &renderer->surfaceHashImageView);

    VkImageView gbufferImageViews[] = {
        renderer->depthImageView,
        renderer->albedoImageView,
        renderer->normalImageView,
        renderer->surfaceHashImageView,
    };
    VkFormat gbufferColorImageFormats[] = {
        renderer->albedoImageFormat,
        renderer->normalImageFormat,
        renderer->surfaceHashImageFormat,
    };
    uint32_t gbufferAttachmentCount
        = sizeof(gbufferImageViews) / sizeof(gbufferImageViews[0]);
    uint32_t gbufferColorAttachmentCount
        = sizeof(gbufferColorImageFormats)
        / sizeof(gbufferColorImageFormats[0]);

    /* GBUFFER SAMPLER */
    {
        VkSamplerCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.magFilter = VK_FILTER_NEAREST;
        createInfo.minFilter = VK_FILTER_NEAREST;
        createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        createInfo.mipLodBias = 0.0f;
        createInfo.anisotropyEnable = VK_FALSE;
        createInfo.maxAnisotropy = 0.0f;
        createInfo.compareEnable = VK_FALSE;
        createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        createInfo.minLod = 0.0f;
        createInfo.maxLod = 0.0f;
        createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        handleVkResult(vkCreateSampler(
                           device->logical,
                           &createInfo,
                           NULL,
                           &renderer->gbufferSampler),
            "creating gbuffer sampler");
    }

    /* SURFACE HASH IRRADIANCE CACHE BUFFER */
    createBuffer(
        device->logical,
        device->physical,
        SURFACE_HASH_IRRADIANCE_CACHE_ELEMENT_COUNT
        * SURFACE_HASH_IRRADIANCE_CACHE_ELEMENT_SIZE,
        0,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &renderer->surfaceHashIrradianceCacheBuffer,
        &renderer->surfaceHashIrradianceCacheBufferMemory);

    /* STORAGE IMAGES */
    createLightAccumulateImage(
        device->logical,
        device->physical,
        device->graphicsQueue,
        device->transientCommandPool,
        renderer->presentExtent,
        &renderer->lightAccumulateImage,
        &renderer->lightAccumulateImageMemory,
        &renderer->lightAccumulateImageView);

    /* CAMERA UNIFORM DESCRIPTOR SET BUFFER */
    createBuffer(
        device->logical,
        device->physical,
        renderer->swapLen * sizeof(CameraUniformData),
        0,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &renderer->cameraUniformBuffer,
        &renderer->cameraUniformBufferMemory);

    /* DESCRIPTOR POOL */
    {
        VkDescriptorPoolSize ubPoolSize;
        ubPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubPoolSize.descriptorCount = renderer->swapLen + 1;

        VkDescriptorPoolSize gbufferPoolSize;
        gbufferPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbufferPoolSize.descriptorCount = gbufferAttachmentCount;

        VkDescriptorPoolSize storageImagePoolSize;
        storageImagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        storageImagePoolSize.descriptorCount = 1;

        VkDescriptorPoolSize storageBufferSize;
        storageBufferSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storageBufferSize.descriptorCount = 1;

        VkDescriptorPoolSize poolSizes[] = {
            ubPoolSize,
            gbufferPoolSize,
            storageImagePoolSize,
            storageBufferSize,
        };

        VkDescriptorPoolCreateInfo poolCreateInfo;
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.pNext = NULL;
        poolCreateInfo.flags = 0;
        poolCreateInfo.maxSets = renderer->swapLen + 3;
        poolCreateInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
        poolCreateInfo.pPoolSizes = poolSizes;

        handleVkResult(
            vkCreateDescriptorPool(
                device->logical,
                &poolCreateInfo,
                NULL,
                &renderer->descriptorPool),
            "creating renderer descriptor pool");
    }

    /* SHADOW VOLUME */
    ivec3 shadowVolumeSize;
    shadowVolumeSize[0] = 256;
    shadowVolumeSize[1] = 256;
    shadowVolumeSize[2] = 256;
    ShadowVolume_init(
        &renderer->shadowVolume,
        device->logical,
        device->physical,
        renderer->descriptorPool,
        device->graphicsQueue,
        device->transientCommandPool,
        shadowVolumeSize);

    /* CAMERA DESCRIPTOR SETS */
    {
        VkDescriptorSetLayoutBinding uboBinding;
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT
            | VK_SHADER_STAGE_FRAGMENT_BIT;
        uboBinding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo.sType
            = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pNext = NULL;
        layoutCreateInfo.flags = 0;
        layoutCreateInfo.bindingCount = 1;
        layoutCreateInfo.pBindings = &uboBinding;

        handleVkResult(
            vkCreateDescriptorSetLayout(
                device->logical,
                &layoutCreateInfo,
                NULL,
                &renderer->cameraDescriptorSetLayout),
            "creating camera descriptor set layout");
    }

    renderer->cameraDescriptorSets = (VkDescriptorSet*)malloc(
        renderer->swapLen * sizeof(VkDescriptorSet));
    allocateDescriptorSets(
        device->logical,
        renderer->cameraDescriptorSetLayout,
        renderer->descriptorPool,
        renderer->swapLen,
        renderer->cameraDescriptorSets);
    for (uint32_t i = 0; i < renderer->swapLen; i++) {
        VkDescriptorBufferInfo bufInfo;
        bufInfo.buffer = renderer->cameraUniformBuffer;
        bufInfo.offset = i * sizeof(CameraUniformData);
        bufInfo.range = sizeof(CameraUniformData);

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = renderer->cameraDescriptorSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pImageInfo = NULL;
        write.pBufferInfo = &bufInfo;
        write.pTexelBufferView = NULL;
        vkUpdateDescriptorSets(device->logical, 1, &write, 0, NULL);
    }

    /* GBUFFER DESCRIPTOR SET */
    createGbufferDescriptorSetLayout(
        device->logical,
        sizeof(gbufferImageViews) / sizeof(gbufferImageViews[0]),
        &renderer->gbufferDescriptorSetLayout);

    allocateDescriptorSets(
        device->logical,
        renderer->gbufferDescriptorSetLayout,
        renderer->descriptorPool,
        1,
        &renderer->gbufferDescriptorSet);
    updateGbufferDescriptorSetBindings(
        device->logical,
        renderer->gbufferSampler,
        sizeof(gbufferImageViews) / sizeof(gbufferImageViews[0]),
        gbufferImageViews,
        renderer->gbufferDescriptorSet);

    /* SURFACE HASH IRRADIANCE CACHE DESCRIPTOR SET */
    {
        VkDescriptorSetLayoutBinding storageBufferBinding;
        storageBufferBinding.binding = 0;
        storageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storageBufferBinding.descriptorCount = 1;
        storageBufferBinding.stageFlags
            = VK_SHADER_STAGE_VERTEX_BIT
            | VK_SHADER_STAGE_FRAGMENT_BIT;
        storageBufferBinding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
        layoutCreateInfo.sType
            = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.pNext = NULL;
        layoutCreateInfo.flags = 0;
        layoutCreateInfo.bindingCount = 1;
        layoutCreateInfo.pBindings = &storageBufferBinding;

        handleVkResult(
            vkCreateDescriptorSetLayout(
                device->logical,
                &layoutCreateInfo,
                NULL,
                &renderer->surfaceHashIrradianceCacheDescriptorSetLayout),
            "creating surface hash irradiance cache descriptor set layout");
    }
    allocateDescriptorSets(
        device->logical,
        renderer->surfaceHashIrradianceCacheDescriptorSetLayout,
        renderer->descriptorPool,
        1,
        &renderer->surfaceHashIrradianceCacheDescriptorSet);
    {
        VkDescriptorBufferInfo storageBufferInfo;
        storageBufferInfo.buffer = renderer->surfaceHashIrradianceCacheBuffer;
        storageBufferInfo.offset = 0;
        storageBufferInfo.range 
            = SURFACE_HASH_IRRADIANCE_CACHE_ELEMENT_COUNT
            * SURFACE_HASH_IRRADIANCE_CACHE_ELEMENT_SIZE;

        VkWriteDescriptorSet write;
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = NULL;
        write.dstSet = renderer->surfaceHashIrradianceCacheDescriptorSet;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pImageInfo = NULL;
        write.pBufferInfo = &storageBufferInfo;
        write.pTexelBufferView = NULL;
        vkUpdateDescriptorSets(device->logical, 1, &write, 0, NULL);
    }

    /* RENDER PASSES */
    createGeometryRenderPass(
        device->logical,
        renderer->depthImageFormat,
        sizeof(gbufferColorImageFormats) / sizeof(gbufferColorImageFormats[0]),
        gbufferColorImageFormats,
        &renderer->geometryRenderPass);
    createLightingRenderPass(
        device->logical,
        renderer->swapImageFormat,
        &renderer->lightingRenderPass);

    /* OBJECT GEOMETRY PIPELINE LAYOUT */
    {
        VkDescriptorSetLayout setLayouts[] = {
            renderer->cameraDescriptorSetLayout,
            renderer->surfaceHashIrradianceCacheDescriptorSetLayout,
            renderer->objStorage.descriptorSetLayout,
        };
        createPipelineLayout(
            device->logical,
            sizeof(setLayouts) / sizeof(setLayouts[0]),
            setLayouts,
            0,
            NULL,
            &renderer->objGeometryPipelineLayout);
    }

    /* OBJECT GEOMETRY PIPELINE */
    {
        VkShaderModule objVertShaderModule, objFragShaderModule;
        createShaderModule(
            device->logical,
            "obj.vert.spv",
            &objVertShaderModule);
        createShaderModule(
            device->logical,
            "obj.frag.spv",
            &objFragShaderModule);
        createObjectGeometryPipeline(
            device->logical,
            renderer->objGeometryPipelineLayout,
            renderer->geometryRenderPass,
            0,
            renderer->presentExtent,
            objVertShaderModule,
            objFragShaderModule,
            gbufferColorAttachmentCount,
            &renderer->objGeometryPipeline);
        vkDestroyShaderModule(
            device->logical,
            objVertShaderModule,
            NULL);
        vkDestroyShaderModule(
            device->logical,
            objFragShaderModule,
            NULL);
    }

    /* LIGHTING PIPELINE LAYOUT */
    {
        VkDescriptorSetLayout setLayouts[] = {
            renderer->cameraDescriptorSetLayout,
            renderer->gbufferDescriptorSetLayout,
            renderer->shadowVolume.descriptorSetLayout,
            renderer->surfaceHashIrradianceCacheDescriptorSetLayout,
        };

        createPipelineLayout(
            device->logical,
            sizeof(setLayouts) / sizeof(setLayouts[0]),
            setLayouts,
            0,
            NULL,
            &renderer->lightingPipelineLayout);
    }

    /* LIGHTING PIPELINE */
    {
        VkShaderModule vertShaderModule, fragShaderModule;
        createShaderModule(
            device->logical,
            "lighting.vert.spv",
            &vertShaderModule);
        createShaderModule(
            device->logical,
            "lighting.frag.spv",
            &fragShaderModule);
        createLightingPipeline(
            device->logical,
            renderer->lightingPipelineLayout,
            renderer->lightingRenderPass,
            0,
            renderer->presentExtent,
            vertShaderModule,
            fragShaderModule,
            &renderer->lightingPipeline);
        vkDestroyShaderModule(
            device->logical,
            vertShaderModule,
            NULL);
        vkDestroyShaderModule(
            device->logical,
            fragShaderModule,
            NULL);
    }

    /* FRAMEBUFFERS */
    renderer->swapImageFramebuffers
        = (VkFramebuffer*)malloc(renderer->swapLen * sizeof(VkFramebuffer));
    createSwapImageFramebuffers(
        device->logical,
        renderer->lightingRenderPass,
        renderer->presentExtent,
        renderer->swapLen,
        renderer->swapImageViews,
        renderer->swapImageFramebuffers);

    createGeometryFramebuffer(
        device->logical,
        renderer->geometryRenderPass,
        renderer->presentExtent,
        sizeof(gbufferImageViews) / sizeof(gbufferImageViews[0]),
        gbufferImageViews,
        &renderer->geometryFramebuffer);

    /* COMMAND BUFFERS */
    renderer->commandBuffers
        = (VkCommandBuffer*)malloc(renderer->swapLen * sizeof(VkCommandBuffer));
    {
        VkCommandBufferAllocateInfo allocInfo;
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = NULL;
        allocInfo.commandPool = device->commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = renderer->swapLen;

        handleVkResult(
            vkAllocateCommandBuffers(
                device->logical,
                &allocInfo,
                renderer->commandBuffers),
            "allocating render command buffers");
    }

    /* SYNCHRONIZATION */
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = NULL;
        semaphoreCreateInfo.flags = 0;

        VkFenceCreateInfo fenceCreateInfo;
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = NULL;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            handleVkResult(
                vkCreateSemaphore(
                    device->logical,
                    &semaphoreCreateInfo,
                    NULL,
                    &renderer->imageAvailableSemaphores[i]),
                "creating image available semaphore");
            handleVkResult(
                vkCreateSemaphore(
                    device->logical,
                    &semaphoreCreateInfo,
                    NULL,
                    &renderer->renderFinishedSemaphores[i]),
                "creating render finished semaphore");
            handleVkResult(
                vkCreateFence(
                    device->logical,
                    &fenceCreateInfo,
                    NULL,
                    &renderer->renderFinishedFences[i]),
                "creating render finished fence");
        }
    }

    renderer->swapchainImageFences
        = (VkFence**)malloc(renderer->swapLen * sizeof(VkFence*));
    memset(
        renderer->swapchainImageFences,
        0,
        renderer->swapLen * sizeof(VkFence*));

    renderer->currentFrame = 0;
    renderer->time = 0;
}

void Renderer_recreateCommandBuffers(
    Renderer* renderer,
    VkDevice logicalDevice)
{
    vkDeviceWaitIdle(logicalDevice);
    for(uint32_t i = 0; i < renderer->swapLen; i++)
        vkResetCommandBuffer(renderer->commandBuffers[i], 0);

    VkClearValue geometryClearValues[1];
    memset(geometryClearValues, 0, sizeof(geometryClearValues));
    geometryClearValues[0].depthStencil.depth = 1.0f;

    VkClearValue lightingClearValues[1];
    memset(lightingClearValues, 0, sizeof(lightingClearValues));
    lightingClearValues[0].color.float32[0] = 128.0f / 255.0;
    lightingClearValues[0].color.float32[1] = 218.0f / 255.0;
    lightingClearValues[0].color.float32[2] = 251.0f / 255.0;
    lightingClearValues[0].color.float32[3] = 1.0f;

    uint32_t objVoxCount = renderer->objStorage.filled;
    VkDescriptorSet* objVoxDescriptorSets = renderer->objStorage.descriptorSets;

    for(uint32_t i = 0; i < renderer->swapLen; i++) {
        VkDescriptorSet objGeometryDescriptorSets[] = {
            renderer->cameraDescriptorSets[i],
            renderer->surfaceHashIrradianceCacheDescriptorSet,
        };
        VkDescriptorSet lightingDescriptorSets[] = {
            renderer->cameraDescriptorSets[i],
            renderer->gbufferDescriptorSet,
            renderer->shadowVolume.descriptorSet,
            renderer->surfaceHashIrradianceCacheDescriptorSet,
        };

        recordRenderCommandBuffer(
            logicalDevice,
            renderer->presentExtent,
            renderer->geometryRenderPass,
            renderer->lightingRenderPass,
            renderer->geometryFramebuffer,
            renderer->swapImageFramebuffers[i],
            sizeof(geometryClearValues) / sizeof(geometryClearValues[0]),
            geometryClearValues,
            sizeof(lightingClearValues) / sizeof(lightingClearValues[0]),
            lightingClearValues,
            renderer->objGeometryPipeline,
            renderer->lightingPipeline,
            renderer->objGeometryPipelineLayout,
            renderer->lightingPipelineLayout,
            sizeof(objGeometryDescriptorSets) 
            / sizeof(objGeometryDescriptorSets[0]),
            objGeometryDescriptorSets,
            sizeof(lightingDescriptorSets) / sizeof(lightingDescriptorSets[0]),
            lightingDescriptorSets,
            objVoxCount,
            objVoxDescriptorSets,
            renderer->commandBuffers[i]);
    }
}

void Renderer_drawFrame(
    Renderer* renderer,
    VulkanDevice* device,
    mat4 view,
    mat4 proj,
    float nearClip,
    float farClip,
    bool movedThisFrame)
{
    handleVkResult(
        vkWaitForFences(
            device->logical,
            1,
            &renderer->renderFinishedFences[renderer->currentFrame],
            VK_TRUE,
            UINT64_MAX),
        "waiting for render finished fence");

    uint32_t imageIndex;
    handleVkResult(
        vkAcquireNextImageKHR(
            device->logical,
            renderer->swapchain,
            UINT64_MAX,
            renderer->imageAvailableSemaphores[renderer->currentFrame],
            VK_NULL_HANDLE,
            &imageIndex),
        "acquiring next swapchain image");

    if (renderer->swapchainImageFences[imageIndex] != NULL)
        handleVkResult(
            vkWaitForFences(
                device->logical,
                1,
                renderer->swapchainImageFences[imageIndex],
                VK_TRUE,
                UINT64_MAX),
            "waiting for swapchain image render finished fence");

    vkResetFences(
        device->logical,
        1,
        &renderer->renderFinishedFences[renderer->currentFrame]);
    for (int i = 0; i < renderer->swapLen; i++)
        if (renderer->swapchainImageFences[i]
            == &renderer->renderFinishedFences[renderer->currentFrame]) {
            renderer->swapchainImageFences[i] = NULL;
            break;
        }
    renderer->swapchainImageFences[imageIndex]
        = &renderer->renderFinishedFences[renderer->currentFrame];

    CameraUniformData* cameraData;
    vkMapMemory(
        device->logical,
        renderer->cameraUniformBufferMemory,
        imageIndex * sizeof(CameraUniformData),
        sizeof(CameraUniformData),
        0,
        (void**)&cameraData);

    glm_mat4_copy(view, cameraData->view);
    glm_mat4_copy(proj, cameraData->proj);
    cameraData->nearClip = nearClip;
    cameraData->farClip = farClip;
    cameraData->time = renderer->time;
    cameraData->movedThisFrame = 0;
    if(movedThisFrame)
        cameraData->movedThisFrame = 1;

    vkUnmapMemory(device->logical, renderer->cameraUniformBufferMemory);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores
        = &renderer->imageAvailableSemaphores[renderer->currentFrame];
    VkPipelineStageFlags waitStage
        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderer->commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores
        = &renderer->renderFinishedSemaphores[renderer->currentFrame];

    handleVkResult(
        vkQueueSubmit(
            device->graphicsQueue,
            1,
            &submitInfo,
            renderer->renderFinishedFences[renderer->currentFrame]),
        "submitting render command buffer");

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = NULL;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores
        = &renderer->renderFinishedSemaphores[renderer->currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &renderer->swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL;

    handleVkResult(
        vkQueuePresentKHR(device->presentQueue, &presentInfo),
        "submitting present call");

    renderer->currentFrame
        = (renderer->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    renderer->time += 1;
}

void Renderer_destroy(
    Renderer* renderer,
    VkDevice logicalDevice)
{
    /* SCENE DATA */
    ObjectStorage_destroy(&renderer->objStorage, logicalDevice);
    ShadowVolume_destroy(&renderer->shadowVolume, logicalDevice);

    /* IMAGES AND FRAMEBUFFERS */
    for (uint32_t i = 0; i < renderer->swapLen; i++) {
        vkDestroyImageView(logicalDevice, renderer->swapImageViews[i], NULL);
        vkDestroyFramebuffer(
            logicalDevice,
            renderer->swapImageFramebuffers[i],
            NULL);
    }
    vkDestroyFramebuffer(
        logicalDevice,
        renderer->geometryFramebuffer,
        NULL);
    free(renderer->swapImages);
    free(renderer->swapImageViews);
    free(renderer->swapImageFramebuffers);
    vkDestroySwapchainKHR(logicalDevice, renderer->swapchain, NULL);

    vkDestroyImage(logicalDevice, renderer->depthImage, NULL);
    vkFreeMemory(logicalDevice, renderer->depthImageMemory, NULL);
    vkDestroyImageView(logicalDevice, renderer->depthImageView, NULL);

    vkDestroyImage(logicalDevice, renderer->albedoImage, NULL);
    vkFreeMemory(logicalDevice, renderer->albedoImageMemory, NULL);
    vkDestroyImageView(logicalDevice, renderer->albedoImageView, NULL);

    vkDestroyImage(logicalDevice, renderer->normalImage, NULL);
    vkFreeMemory(logicalDevice, renderer->normalImageMemory, NULL);
    vkDestroyImageView(logicalDevice, renderer->normalImageView, NULL);

    vkDestroyImage(logicalDevice, renderer->surfaceHashImage, NULL);
    vkFreeMemory(logicalDevice, renderer->surfaceHashImageMemory, NULL);
    vkDestroyImageView(logicalDevice, renderer->surfaceHashImageView, NULL);

    vkDestroySampler(logicalDevice, renderer->gbufferSampler, NULL);

    vkDestroyImage(logicalDevice, renderer->lightAccumulateImage, NULL);
    vkFreeMemory(logicalDevice, renderer->lightAccumulateImageMemory, NULL);
    vkDestroyImageView(logicalDevice, renderer->lightAccumulateImageView, NULL);

    /* STORAGE BUFFERS */
    vkDestroyBuffer(
        logicalDevice,
        renderer->surfaceHashIrradianceCacheBuffer,
        NULL);
    vkFreeMemory(
        logicalDevice,
        renderer->surfaceHashIrradianceCacheBufferMemory,
        NULL);

    /* UNIFORM BUFFERS */
    vkDestroyBuffer(logicalDevice, renderer->cameraUniformBuffer, NULL);
    vkFreeMemory(logicalDevice, renderer->cameraUniformBufferMemory, NULL);

    /* DESCRIPTOR SETS */
    vkDestroyDescriptorPool(
        logicalDevice,
        renderer->descriptorPool,
        NULL);
    vkDestroyDescriptorSetLayout(
        logicalDevice,
        renderer->cameraDescriptorSetLayout,
        NULL);
    free(renderer->cameraDescriptorSets);
    vkDestroyDescriptorSetLayout(
        logicalDevice,
        renderer->gbufferDescriptorSetLayout,
        NULL);
    vkDestroyDescriptorSetLayout(
        logicalDevice,
        renderer->surfaceHashIrradianceCacheDescriptorSetLayout,
        NULL);

    /* PIPELINES AND RENDER PASSES */
    vkDestroyPipelineLayout(
        logicalDevice,
        renderer->objGeometryPipelineLayout,
        NULL);
    vkDestroyPipeline(logicalDevice, renderer->objGeometryPipeline, NULL);
    vkDestroyPipelineLayout(
        logicalDevice,
        renderer->lightingPipelineLayout,
        NULL);
    vkDestroyPipeline(logicalDevice, renderer->lightingPipeline, NULL);
    vkDestroyRenderPass(logicalDevice, renderer->geometryRenderPass, NULL);
    vkDestroyRenderPass(logicalDevice, renderer->lightingRenderPass, NULL);

    /* COMMAND BUFFERS */
    free(renderer->commandBuffers);

    /* SYNCHRONIZATION */
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(logicalDevice, renderer->imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(logicalDevice, renderer->renderFinishedSemaphores[i], NULL);
        vkDestroyFence(logicalDevice, renderer->renderFinishedFences[i], NULL);
    }
    free(renderer->swapchainImageFences);
}
