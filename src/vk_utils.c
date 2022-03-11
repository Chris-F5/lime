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
    VkDeviceMemory* imageMemory)
{
    VkImageCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = flags;
    createInfo.imageType = imageType;
    createInfo.format = format;
    createInfo.extent = extent;
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = arrayLayers;
    createInfo.samples = samples;
    createInfo.tiling = tiling;
    createInfo.usage = usageFlags;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
    createInfo.initialLayout = preinitialized 
        ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

    handleVkResult(
        vkCreateImage(device, &createInfo, NULL, image),
        "creating image");

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, *image, &memReq);

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
        vkBindImageMemory(device, *image, *imageMemory, 0),
        "binding image memory");
}

void createShaderModule(
    VkDevice device,
    const char* srcFileName,
    VkShaderModule* shaderModule)
{
    FILE *file = fopen(srcFileName, "rb");
    if (!file) {
        printf("Exiting because file '%s' could not be opened\n", srcFileName);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    long spvLength = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* spv = (char *)malloc(spvLength);
    fread(spv, 1, spvLength, file);
    fclose(file);

    VkShaderModuleCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.codeSize = spvLength;
    createInfo.pCode = (uint32_t *)spv;

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
