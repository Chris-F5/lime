#include "./vk_utils.h"

#include <stdio.h>
#include <stdlib.h>

static char* vkResultToString(VkResult result)
{
    switch (result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
        return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
        return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_EXT:
        return "VK_ERROR_NOT_PERMITTED_EXT";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    default:
        return "UNKNNOWN VULKAN ERROR";
    }
}

void handleVkResult(VkResult result, char* message)
{
    if (result != VK_SUCCESS) {
        printf(
            "Exiting because of vulkan error:\n\terror:'%s'\n\tmessage: '%s'\n",
            vkResultToString(result),
            message);
        exit(EXIT_FAILURE);
    }
}

uint32_t findPhysicalDeviceMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t memoryTypeBits,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
        if ((memoryTypeBits & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties)
                == properties)
            return i;

    puts("Exiting because failed to find requested physical devicememory type");
    exit(EXIT_FAILURE);
}

void createBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferCreateFlags flags,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkBuffer* buffer,
    VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = flags;
    createInfo.size = size;
    createInfo.usage = usageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;

    handleVkResult(
        vkCreateBuffer(device, &createInfo, NULL, buffer),
        "creating buffer");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, *buffer, &memReq);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findPhysicalDeviceMemoryType(
        physicalDevice,
        memReq.memoryTypeBits,
        memoryPropertyFlags);

    handleVkResult(
        vkAllocateMemory(device, &allocInfo, NULL, bufferMemory),
        "allocating buffer");

    handleVkResult(
        vkBindBufferMemory(device, *buffer, *bufferMemory, 0),
        "binding buffer meomry");
}

void allocateImageMemory(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkImage image,
    VkDeviceMemory* imageMemory)
{
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, image, &memReq);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = NULL;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findPhysicalDeviceMemoryType(
        physicalDevice,
        memReq.memoryTypeBits,
        memoryPropertyFlags);

    handleVkResult(
        vkAllocateMemory(device, &allocInfo, NULL, imageMemory),
        "allocating image");

    handleVkResult(
        vkBindImageMemory(device, image, *imageMemory, 0),
        "binding image memory");
}

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
    VkPipelineStageFlags dstStageMask)
{
    VkCommandBuffer commandBuffer;

    /* ALLOCATE AND RECORD COMMAND BUFFER */
    {
        VkCommandBufferAllocateInfo commandBufferAllocInfo;
        commandBufferAllocInfo.sType
            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocInfo.pNext = NULL;
        commandBufferAllocInfo.commandPool = commandPool;
        commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocInfo.commandBufferCount = 1;

        handleVkResult(
            vkAllocateCommandBuffers(
                device,
                &commandBufferAllocInfo,
                &commandBuffer),
            "allocating image layout transition command buffer");

        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = NULL;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        handleVkResult(
            vkBeginCommandBuffer(
                commandBuffer,
                &beginInfo),
            "begin recording image layout transition command buffer");

        VkImageMemoryBarrier barrier;
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = NULL;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = imageSubresourceRange;

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStageMask, dstStageMask,
            0,
            0, NULL,
            0, NULL,
            1, &barrier);

        handleVkResult(
            vkEndCommandBuffer(commandBuffer),
            "ending layout transition command buffer");
    }

    /* SUBMIT COMMAND BUFFER */

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;
    vkQueueSubmit(
        queue,
        1,
        &submitInfo,
        VK_NULL_HANDLE);

    handleVkResult(
        vkQueueWaitIdle(queue),
        "waiting queue idle for image layout transition");

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void copyBufferToGeneralColorImage(
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue queue,
    VkBuffer buffer,
    VkImage image,
    VkExtent3D imageExtent)
{
    VkCommandBuffer commandBuffer;

    /* ALLOCATE AND RECORD COMMAND BUFFER */
    {
        VkCommandBufferAllocateInfo commandBufferAllocInfo;
        commandBufferAllocInfo.sType
            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocInfo.pNext = NULL;
        commandBufferAllocInfo.commandPool = commandPool;
        commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocInfo.commandBufferCount = 1;

        handleVkResult(
            vkAllocateCommandBuffers(
                device,
                &commandBufferAllocInfo,
                &commandBuffer),
            "allocating buffer to image copy command buffer");

        VkCommandBufferBeginInfo beginInfo;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = NULL;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        handleVkResult(
            vkBeginCommandBuffer(
                commandBuffer,
                &beginInfo),
            "begin recording buffer to image copy command buffer");

        VkImageSubresourceLayers subresource;
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.mipLevel = 0;
        subresource.baseArrayLayer = 0;
        subresource.layerCount = 1;

        VkBufferImageCopy copy;
        copy.bufferOffset = 0;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource = subresource;
        copy.imageOffset = (VkOffset3D) { 0, 0, 0 };
        copy.imageExtent = imageExtent;

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_GENERAL,
            1,
            &copy);

        handleVkResult(
            vkEndCommandBuffer(commandBuffer),
            "ending buffer image copy command buffer");
    }

    /* SUBMIT COMMAND BUFFER */

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = NULL;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = NULL;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = NULL;
    vkQueueSubmit(
        queue,
        1,
        &submitInfo,
        VK_NULL_HANDLE);

    handleVkResult(
        vkQueueWaitIdle(queue),
        "waiting queue idle for buffer image copy");

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void createShaderModule(
    VkDevice device,
    const char* srcFileName,
    VkShaderModule* shaderModule)
{
    FILE* file = fopen(srcFileName, "rb");
    if (!file) {
        printf("Exiting because file '%s' could not be opened\n", srcFileName);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    long spvLength = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* spv = (char*)malloc(spvLength);
    fread(spv, 1, spvLength, file);
    fclose(file);

    VkShaderModuleCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.codeSize = spvLength;
    createInfo.pCode = (uint32_t*)spv;

    handleVkResult(
        vkCreateShaderModule(device, &createInfo, NULL, shaderModule),
        "creating shader module");
    free(spv);
}

void allocateDescriptorSets(
    VkDevice device,
    VkDescriptorSetLayout layout,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    VkDescriptorSet* descriptorSets)
{
    VkDescriptorSetLayout* setLayoutCopies
        = (VkDescriptorSetLayout*)malloc(count * sizeof(VkDescriptorSetLayout));
    for (int i = 0; i < count; i++)
        setLayoutCopies[i] = layout;

    VkDescriptorSetAllocateInfo setAllocInfo;
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.pNext = NULL;
    setAllocInfo.descriptorPool = descriptorPool;
    setAllocInfo.descriptorSetCount = count;
    setAllocInfo.pSetLayouts = setLayoutCopies;

    handleVkResult(
        vkAllocateDescriptorSets(device, &setAllocInfo, descriptorSets),
        "allocating descriptor sets");

    free(setLayoutCopies);
}

void createPipelineLayout(
    VkDevice logicalDevice,
    uint32_t descriptorSetLayoutCount,
    VkDescriptorSetLayout* descriptorSetLayouts,
    uint32_t pushConstantRangeCount,
    VkPushConstantRange* pushConstantRanges,
    VkPipelineLayout* pipelineLayout)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType
        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = NULL;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayoutCount;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges;

    handleVkResult(
        vkCreatePipelineLayout(
            logicalDevice,
            &pipelineLayoutCreateInfo,
            NULL,
            pipelineLayout),
        "creating pipeline layout");
}
