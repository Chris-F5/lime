#ifndef LIME_SCREEN_IMAGES_H
#define LIME_SCREEN_IMAGES_H

#include <vulkan/vulkan.h>

void createSwapchain(
    VkDevice logicalDevice,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR surfaceFormat,
    VkSurfaceCapabilitiesKHR surfaceCapabilities,
    VkPresentModeKHR presentMode,
    uint32_t graphicsFamilyIndex,
    uint32_t presentFamilyIndex,
    VkExtent2D extent,
    VkSwapchainKHR* swapchain);

void createDepthImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkFormat depthImageFormat,
    VkExtent2D extent,
    VkImage* depthImage,
    VkDeviceMemory* depthImageMemory,
    VkImageView* depthImageView);

void createColorAttachmentImage(
    VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice,
    VkFormat imageFormat,
    VkExtent2D extent,
    VkImage* image,
    VkDeviceMemory* imageMemory,
    VkImageView* imageView);

#endif
