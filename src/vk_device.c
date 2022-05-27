#include "./vk_device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vk_utils.h"

static const uint32_t QUEUE_FAMILY_DOES_NOT_EXIST = UINT32_MAX;

static const char* VALIDATION_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char* REQUIRED_DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkResult createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func
        = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance,
            "vkCreateDebugUtilsMessengerEXT");
    if (func == NULL) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    } else {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
}

static VkResult destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func
        = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance,
            "vkDestroyDebugUtilsMessengerEXT");
    if (func == NULL) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    } else {
        func(instance, debugMessenger, pAllocator);
        return VK_SUCCESS;
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL validationLayerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf("validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

void createDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT* debugMessenger)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.messageSeverity
        =  /* VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | */
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType
        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = validationLayerCallback;
    createInfo.pUserData = NULL;

    handleVkResult(
        createDebugUtilsMessengerEXT(
            instance,
            &createInfo,
            NULL,
            debugMessenger),
        "creating debug messenger");
}

void destroyDebugMessenger(
    VkInstance instance,
    VkDebugUtilsMessengerEXT* debugMessenger)
{
    handleVkResult(
        destroyDebugUtilsMessengerEXT(
            instance,
            *debugMessenger,
            NULL),
        "destroying debug messenger");
}

static bool checkValidationLayerSupport(void)
{
    uint32_t availableLayerCount;
    handleVkResult(
        vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL),
        "getting instance layer properties count to check for validation layer \
        support");
    VkLayerProperties* availableLayers = (VkLayerProperties*)malloc(
        availableLayerCount * sizeof(VkLayerProperties));
    handleVkResult(
        vkEnumerateInstanceLayerProperties(
            &availableLayerCount,
            availableLayers),
        "getting instance layer properties to check for validation layer \
        support");

    const int validationLayerCount
        = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);
    for (int vi = 0; vi < validationLayerCount; vi++) {
        bool layerFound = false;
        for (int ai = 0; ai < availableLayerCount; ai++)
            if (strcmp(
                    VALIDATION_LAYERS[vi],
                    availableLayers[ai].layerName)
                == 0) {
                layerFound = true;
                break;
            }

        if (!layerFound) {
            free(availableLayers);
            return false;
        }
    }

    free(availableLayers);
    return true;
}

static void createInstance(
    char* appName,
    uint32_t appVersion,
    uint32_t vulkanApiVersion,
    bool validationLayersEnabled,
    VkInstance* instance)
{
    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = NULL;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = appVersion;
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vulkanApiVersion;

    uint32_t glfwExtensionCount;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    uint32_t enabledExtensionCount;
    const char** enabledExtensions;
    if (validationLayersEnabled) {
        enabledExtensionCount = glfwExtensionCount + 1;
        enabledExtensions = (const char**)malloc(
            enabledExtensionCount * sizeof(char*));
        memcpy(
            enabledExtensions,
            glfwExtensions,
            glfwExtensionCount * sizeof(char*));
        enabledExtensions[glfwExtensionCount]
            = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    } else {
        enabledExtensionCount = glfwExtensionCount;
        enabledExtensions = glfwExtensions;
    }

    VkInstanceCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = enabledExtensionCount;
    createInfo.ppEnabledExtensionNames = enabledExtensions;

    if (validationLayersEnabled) {
        createInfo.enabledLayerCount
            = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = NULL;
    }

    handleVkResult(
        vkCreateInstance(&createInfo, NULL, instance),
        "creating vk instance");
}

static bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
    uint32_t availableExtensionCount;
    handleVkResult(
        vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            NULL,
            &availableExtensionCount,
            NULL),
        "getting physical device extension count to check for physical device \
        extension support");
    VkExtensionProperties* availableExtensions = (VkExtensionProperties*)malloc(
        availableExtensionCount * sizeof(VkExtensionProperties));
    handleVkResult(
        vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            NULL,
            &availableExtensionCount,
            availableExtensions),
        "getting physical device extensions to check for physical device \
        extension support");

    for (
        uint32_t ri = 0;
        ri < sizeof(REQUIRED_DEVICE_EXTENSIONS)
            / sizeof(REQUIRED_DEVICE_EXTENSIONS[0]);
        ri++) {
        bool layerFound = false;
        for (uint32_t ai = 0; ai < availableExtensionCount; ai++)
            if (strcmp(
                    REQUIRED_DEVICE_EXTENSIONS[ri],
                    availableExtensions[ai].extensionName)
                == 0) {
                layerFound = true;
                break;
            }
        if (!layerFound) {
            free(availableExtensions);
            return false;
        }
    }
    free(availableExtensions);
    return true;
}

static bool chooseSwapchainSurfaceFormat(
    uint32_t availableFormatCount,
    VkSurfaceFormatKHR* availableFormats,
    VkSurfaceFormatKHR* chosenFormat)
{
    for (uint32_t i = 0; i < availableFormatCount; i++)
        if (
            availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM
            && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            *chosenFormat = availableFormats[i];
            return true;
        }
    return false;
}

static bool chooseSwapchainPresentMode(
    uint32_t availablePresentModeCount,
    VkPresentModeKHR* availablePresentModes,
    VkPresentModeKHR* chosenPresentMode)
{
    for (uint32_t i = 0; i < availablePresentModeCount; i++)
        if (availablePresentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
            *chosenPresentMode = availablePresentModes[i];
            return true;
        }
    for (uint32_t i = 0; i < availablePresentModeCount; i++)
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            *chosenPresentMode = availablePresentModes[i];
            return true;
        }
    return false;
}

static bool checkPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    PhysicalDeviceProperties* physicalDeviceProperties)
{
    /* QUEUE FAMILIES */
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueFamilyCount,
        NULL);

    VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(
        sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueFamilyCount,
        queueFamilies);

    bool foundGraphicsFamily = false;
    bool foundPresentFamily = false;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        // TODO: seperate compute queue?
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            physicalDeviceProperties->graphicsFamilyIndex = i;
            foundGraphicsFamily = true;
        }
        VkBool32 presentSupport;
        handleVkResult(
            vkGetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice,
                i,
                surface,
                &presentSupport),
            "checking queue family surface support to see if its suitable for \
            a present family");
        if (presentSupport) {
            physicalDeviceProperties->presentFamilyIndex = i;
            foundPresentFamily = true;
        }
    }

    if (!foundGraphicsFamily || !foundPresentFamily)
        return false;

    free(queueFamilies);

    /* EXTENSIONS */
    if (!checkDeviceExtensionSupport(physicalDevice))
        return false;

    /* SURFACE CAPABILITIES */
    handleVkResult(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physicalDevice,
            surface,
            &physicalDeviceProperties->surfaceCapabilities),
        "getting physical device surface capabilities");

    /* SURFACE FORMATS */
    uint32_t surfaceFormatCount;
    handleVkResult(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &surfaceFormatCount,
            NULL),
        "getting physical device surface formats count");
    VkSurfaceFormatKHR* surfaceFormats = (VkSurfaceFormatKHR*)malloc(
        surfaceFormatCount * sizeof(VkSurfaceFormatKHR));
    handleVkResult(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &surfaceFormatCount,
            surfaceFormats),
        "getting physical device surface formats");

    if (!chooseSwapchainSurfaceFormat(
            surfaceFormatCount,
            surfaceFormats,
            &physicalDeviceProperties->surfaceFormat))
        return false;

    /* PRESENT MODES */
    uint32_t presentModeCount;
    handleVkResult(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &presentModeCount,
            NULL),
        "getting surface present modes count for swapchain creation");
    VkPresentModeKHR* presentModes = (VkPresentModeKHR*)malloc(
        presentModeCount * sizeof(VkPresentModeKHR));
    handleVkResult(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface,
            &presentModeCount,
            presentModes),
        "getting surface present modes for swapchain creation");

    if (!chooseSwapchainPresentMode(
            presentModeCount,
            presentModes,
            &physicalDeviceProperties->presentMode))
        return false;

    /* DEPTH IMAGE FORMATS */
    physicalDeviceProperties->depthImageFormat = VK_FORMAT_D32_SFLOAT;
    VkFormatProperties depthImageFormatProperties;
    vkGetPhysicalDeviceFormatProperties(
        physicalDevice,
        physicalDeviceProperties->depthImageFormat,
        &depthImageFormatProperties);
    if ((depthImageFormatProperties.optimalTilingFeatures
            & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        == 0)
        return false;

    return true;
}

static void selectPhysicalDevice(
    VkInstance instance,
    VkSurfaceKHR surface,
    VkPhysicalDevice* physicalDevice,
    PhysicalDeviceProperties* physicalDeviceProperties)
{
    uint32_t physicalDeviceCount;
    handleVkResult(
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL),
        "getting physical device count");
    if (physicalDeviceCount == 0) {
        puts("Exiting because could not find GPU with vulkan support");
        exit(EXIT_FAILURE);
    }
    VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*)malloc(
        physicalDeviceCount * sizeof(VkPhysicalDevice));
    handleVkResult(
        vkEnumeratePhysicalDevices(
            instance,
            &physicalDeviceCount,
            physicalDevices),
        "enumerating physical devices");

    for (uint32_t i = 0; i < physicalDeviceCount; i++)
        if (checkPhysicalDeviceProperties(
                physicalDevices[i],
                surface,
                physicalDeviceProperties)) {
            *physicalDevice = physicalDevices[i];
            free(physicalDevices);
            return;
        }
    free(physicalDevices);

    puts("Exiting because could not find suitable GPU");
    exit(EXIT_FAILURE);
}

static void createLogicalDevice(
    VkPhysicalDevice physicalDevice,
    PhysicalDeviceProperties physicalDeviceProperties,
    bool validationLayersEnabled,
    VkDevice* logicalDevice)
{
    uint32_t queueFamilyCount;
    uint32_t queueFamilyIndices[2];

    if (physicalDeviceProperties.graphicsFamilyIndex
        == physicalDeviceProperties.presentFamilyIndex) {
        queueFamilyCount = 1;
        queueFamilyIndices[0] = physicalDeviceProperties.graphicsFamilyIndex;
    } else {
        queueFamilyCount = 2;
        queueFamilyIndices[0] = physicalDeviceProperties.graphicsFamilyIndex;
        queueFamilyIndices[1] = physicalDeviceProperties.presentFamilyIndex;
    }

    VkDeviceQueueCreateInfo* queueCreateInfos
        = (VkDeviceQueueCreateInfo*)malloc(
            queueFamilyCount * sizeof(VkDeviceQueueCreateInfo));

    float generalQueuePriority = 1.0;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].pNext = NULL;
        queueCreateInfos[i].flags = 0;
        queueCreateInfos[i].queueFamilyIndex = queueFamilyIndices[i];
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].pQueuePriorities = &generalQueuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures;
    memset(&deviceFeatures, 0, sizeof(deviceFeatures));

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = NULL;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = queueFamilyCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    if (validationLayersEnabled) {
        deviceCreateInfo.enabledLayerCount
            = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);
        deviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = NULL;
    }
    deviceCreateInfo.enabledExtensionCount
        = sizeof(REQUIRED_DEVICE_EXTENSIONS)
        / sizeof(REQUIRED_DEVICE_EXTENSIONS[0]);
    deviceCreateInfo.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    handleVkResult(
        vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, logicalDevice),
        "creating logical device");
}

void VulkanDevice_init(VulkanDevice* device, GLFWwindow* window)
{
#ifdef DEBUG
    device->validationLayersEnabled = checkValidationLayerSupport();
    if (device->validationLayersEnabled)
        puts("Validation layers enabled.");
    else
        puts("Debug mode on but validation layers are not supported.");
#else
    device->validationLayersEnabled = false;
#endif

    createInstance(
        "SPGE",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0,
        device->validationLayersEnabled,
        &device->instance);

    if (device->validationLayersEnabled) {
        createDebugMessenger(device->instance, &device->debugMessenger);
    } else {
        device->debugMessenger = VK_NULL_HANDLE;
    }

    handleVkResult(
        glfwCreateWindowSurface(
            device->instance,
            window,
            NULL,
            &device->surface),
        "creating surface");

    selectPhysicalDevice(
        device->instance,
        device->surface,
        &device->physical,
        &device->physicalProperties);

    createLogicalDevice(
        device->physical,
        device->physicalProperties,
        device->validationLayersEnabled,
        &device->logical);

    vkGetDeviceQueue(
        device->logical,
        device->physicalProperties.graphicsFamilyIndex,
        0,
        &device->graphicsQueue);

    vkGetDeviceQueue(
        device->logical,
        device->physicalProperties.presentFamilyIndex,
        0,
        &device->presentQueue);

    {
        VkCommandPoolCreateInfo poolCreateInfo;
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.pNext = NULL;
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolCreateInfo.queueFamilyIndex 
            = device->physicalProperties.graphicsFamilyIndex;

        handleVkResult(
            vkCreateCommandPool(
                device->logical,
                &poolCreateInfo,
                NULL,
                &device->commandPool),
            "creating command pool");
    }
    {
        VkCommandPoolCreateInfo poolCreateInfo;
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.pNext = NULL;
        poolCreateInfo.flags 
            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 
            | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolCreateInfo.queueFamilyIndex 
            = device->physicalProperties.graphicsFamilyIndex;

        handleVkResult(
            vkCreateCommandPool(
                device->logical,
                &poolCreateInfo,
                NULL,
                &device->transientCommandPool),
            "creating transient command pool");
    }
}

void VulkanDevice_destroy(VulkanDevice* device)
{
    vkDestroyCommandPool(device->logical, device->commandPool, NULL);
    vkDestroyCommandPool(device->logical, device->transientCommandPool, NULL);
    vkDestroyDevice(device->logical, NULL);
    vkDestroySurfaceKHR(device->instance, device->surface, NULL);
    destroyDebugMessenger(device->instance, &device->debugMessenger);
    vkDestroyInstance(device->instance, NULL);
}
