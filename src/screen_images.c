#include "./screen_images.h"

#include "vk_utils.h"
#include "utils.h"

void createSwapchain(
    VkDevice logicalDevice,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR surfaceFormat,
    VkSurfaceCapabilitiesKHR surfaceCapabilities,
    VkPresentModeKHR presentMode,
    uint32_t graphicsFamilyIndex,
    uint32_t presentFamilyIndex,
    VkExtent2D extent,
    VkSwapchainKHR* swapchain)
{
    uint32_t minSwapLen = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount != 0)
        minSwapLen = MIN(
            minSwapLen,
            surfaceCapabilities.maxImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = NULL;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = minSwapLen;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphicsFamilyIndex != presentFamilyIndex) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        uint32_t queueFamilyIndices[] = {
            graphicsFamilyIndex,
            presentFamilyIndex
        };
        swapchainCreateInfo.queueFamilyIndexCount
            = sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]);
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = NULL;
    }
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    handleVkResult(
        vkCreateSwapchainKHR(
            logicalDevice,
            &swapchainCreateInfo,
            NULL,
            swapchain),
        "creating swapchain");
}

void createDepthImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkFormat depthImageFormat,
    VkExtent2D extent,
    VkImage* depthImage,
    VkDeviceMemory* depthImageMemory,
    VkImageView* depthImageView)
{
    VkExtent3D extent3D;
    extent3D.width = extent.width;
    extent3D.height = extent.height;
    extent3D.depth = 1;

    VkImageCreateInfo imageCreateInfo;
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = NULL;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = depthImageFormat;
    imageCreateInfo.extent = extent3D;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage
        = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = NULL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    handleVkResult(
        vkCreateImage(
            logicalDevice,
            &imageCreateInfo,
            NULL,
            depthImage),
        "creating depth image");

    allocateImageMemory(
        logicalDevice,
        physicalDevice,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        *depthImage,
        depthImageMemory);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    VkImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.pNext = NULL;
    viewCreateInfo.flags = 0;
    viewCreateInfo.image = *depthImage;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = depthImageFormat;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange = subresourceRange;
    handleVkResult(
        vkCreateImageView(
            logicalDevice,
            &viewCreateInfo,
            NULL,
            depthImageView),
        "creating depth image view");
}

void createColorAttachmentImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkFormat imageFormat,
    VkExtent2D extent,
    VkImage* image,
    VkDeviceMemory* imageMemory,
    VkImageView* imageView)
{
    VkExtent3D extent3D;
    extent3D.width = extent.width;
    extent3D.height = extent.height;
    extent3D.depth = 1;

    VkImageCreateInfo imageCreateInfo;
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = NULL;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = imageFormat;
    imageCreateInfo.extent = extent3D;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage
        = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_SAMPLED_BIT;
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
        "creating color attachment image");

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

    VkImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.pNext = NULL;
    viewCreateInfo.flags = 0;
    viewCreateInfo.image = *image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = imageFormat;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange = subresourceRange;
    handleVkResult(
        vkCreateImageView(
            logicalDevice,
            &viewCreateInfo,
            NULL,
            imageView),
        "creating color attachment image view");
}

void createLightAccumulateImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkQueue graphicsQueue,
    VkCommandPool transientCommandPool,
    VkExtent2D extent,
    VkImage* image,
    VkDeviceMemory* imageMemory,
    VkImageView* imageView)
{
    VkFormat format = VK_FORMAT_R16G16_UINT;

    VkExtent3D extent3D;
    extent3D.width = extent.width;
    extent3D.height = extent.height;
    extent3D.depth = 1;

    VkImageCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = extent3D;
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    vkCreateImage(
        logicalDevice,
        &createInfo,
        NULL,
        image);

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

    VkImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.pNext = NULL;
    viewCreateInfo.flags = 0;
    viewCreateInfo.image = *image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange = subresourceRange;
    handleVkResult(
        vkCreateImageView(
            logicalDevice,
            &viewCreateInfo,
            NULL,
            imageView),
        "creating light accumulate image view");

    transitionImageLayout(
        logicalDevice,
        graphicsQueue,
        transientCommandPool,
        *image,
        subresourceRange,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        0,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}
