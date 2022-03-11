#ifndef LIME_VK_DEVICE_H
#define LIME_VK_DEVICE_H

#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

typedef struct {
    uint32_t graphicsFamilyIndex;
    uint32_t presentFamilyIndex;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;

    VkFormat depthImageFormat;

    VkExtent2D presentExtent;
} PhysicalDeviceProperties;

typedef struct {
    bool validationLayersEnabled;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical;
    PhysicalDeviceProperties physicalProperties;
    VkDevice logical;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkCommandPool commandPool;
    VkCommandPool transientCommandPool;
} VulkanDevice;

void VulkanDevice_init(VulkanDevice* device, GLFWwindow* window);

void VulkanDevice_destroy(VulkanDevice* device);


#endif
