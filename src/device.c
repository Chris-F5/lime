#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"
#include "utils.h"

static int check_validation_layer_support(void);
static void create_instance(int validation_layers_enabled);
static VKAPI_ATTR VkBool32 VKAPI_CALL validation_layer_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);
static void create_debug_messenger(void);
static int check_physical_device_extension_support(VkPhysicalDevice physical_device);
static void select_physical_device(void);
static void select_queue_family(void);
static void create_device(void);

static const char *VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
static const char * const EXTENSIONS[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkInstance instance;
static VkDebugUtilsMessengerEXT debug_messenger;
static VkPhysicalDevice physical_device;
static VkPhysicalDeviceProperties physical_device_properties;
static VkPhysicalDeviceMemoryProperties memory_properties;

struct lime_device lime_device;

static int
check_validation_layer_support(void)
{
  VkResult err;
  uint32_t available_layer_count;
  VkLayerProperties *available_layers;
  int i;

  err = vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating instance layer properties");
    exit(1);
  }
  available_layers = xmalloc(available_layer_count * sizeof(VkLayerProperties));
  err = vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating instance layer properties");
    exit(1);
  }

  for (i = 0; i < available_layer_count; i++) {
    if (strcmp(available_layers[i].layerName, VALIDATION_LAYER) == 0) {
      free(available_layers);
      return 1;
    }
  }
  free(available_layers);
  return 0;
}

static void
create_instance(int validation_layers_enabled)
{
  uint32_t glfw_extension_count, extension_count;
  const char **glfw_extensions, **extensions;
  VkApplicationInfo app_info;
  VkInstanceCreateInfo create_info;
  VkResult err;

  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  if (validation_layers_enabled) {
    extension_count = glfw_extension_count + 1;
    extensions = xmalloc(extension_count * sizeof(char *));
    memcpy(extensions, glfw_extensions, glfw_extension_count * sizeof(char *));
    extensions[glfw_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  } else {
    extension_count = glfw_extension_count;
    extensions = glfw_extensions;
  }

  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = NULL;
  app_info.pApplicationName = "demo_renderer";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pEngineName = "lime";
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.apiVersion = VK_API_VERSION_1_0;

  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = extension_count;
  create_info.ppEnabledExtensionNames = extensions;
  if (validation_layers_enabled) {
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = &VALIDATION_LAYER;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;
  }

  assert(instance == VK_NULL_HANDLE);
  err = vkCreateInstance(&create_info, NULL, &instance);
  ASSERT_VK_RESULT(err, "creating instance");

  if (extensions != glfw_extensions)
    free(extensions);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL validation_layer_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    fprintf(stderr, "validation layer: %s\n", callback_data->pMessage);
    return VK_FALSE;
}

static void
create_debug_messenger(void)
{
  VkDebugUtilsMessengerCreateInfoEXT create_info;
  PFN_vkCreateDebugUtilsMessengerEXT create_func;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.messageSeverity
    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageType
    = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.pfnUserCallback = validation_layer_callback;
  create_info.pUserData = NULL;

  create_func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT");
  assert(create_func);
  assert(debug_messenger == VK_NULL_HANDLE);
  err = create_func(instance, &create_info, NULL, &debug_messenger);
  ASSERT_VK_RESULT(err, "creating debug utils messenger");
}

static int
check_physical_device_extension_support(VkPhysicalDevice physical_device)
{
  VkResult err;
  uint32_t available_extension_count;
  int r, a;
  VkExtensionProperties *available_extensions;

  err = vkEnumerateDeviceExtensionProperties(physical_device, NULL,
      &available_extension_count, NULL);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating available physical device extensions");
    exit(1);
  }
  available_extensions = xmalloc(
      available_extension_count * sizeof(VkExtensionProperties));
  err = vkEnumerateDeviceExtensionProperties(physical_device, NULL,
      &available_extension_count, available_extensions);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating available physical device extensions");
    exit(1);
  }
  for (r = 0; r < sizeof(EXTENSIONS) / sizeof(EXTENSIONS[0]); r++) {
    for (a = 0; a < available_extension_count; a++)
      if (strcmp(EXTENSIONS[r],
            available_extensions[a].extensionName) == 0)
        break;
    if (a == available_extension_count)
      return 0;
  }
  return 1;
}

static void
select_physical_device(void)
{
  uint32_t count;
  VkPhysicalDevice *physical_devices;
  VkResult err;
  err = vkEnumeratePhysicalDevices(instance, &count, NULL);
  ASSERT_VK_RESULT(err, "enumerating physical devices");
  physical_devices = xmalloc(count * sizeof(VkPhysicalDevice));
  err = vkEnumeratePhysicalDevices(instance, &count, physical_devices);
  ASSERT_VK_RESULT(err, "enumerating physical devices");
  if (count == 0) {
    fprintf(stderr, "No physical devices with vulkan support.\n");
    exit(1);
  }
  assert(physical_device == VK_NULL_HANDLE);
  physical_device = physical_devices[0];
  vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
  if (!check_physical_device_extension_support(physical_device)) {
    fprintf(stderr, "Physical device does not support required extensions.\n");
    exit(1);
  }
  free(physical_devices);
}

static void
select_queue_family(void)
{
  uint32_t count;
  VkQueueFamilyProperties *properties;
  int i;
  VkResult err;
  VkBool32 surface_support;

  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);
  properties = xmalloc(count * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties);

  lime_device.graphics_family_index = count;
  for (i = 0; i < count; i++) {
    err = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i,
        lime_device.surface, &surface_support);
    ASSERT_VK_RESULT(err, "getting queue family surface support");
    if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && surface_support) {
      lime_device.graphics_family_index = i;
      break;
    }
  }
  free(properties);
  if (lime_device.graphics_family_index == count) {
    fprintf(stderr, "No graphics queue family found.\n");
    exit(1);
  }
}

static void
create_device(void)
{
  VkDeviceCreateInfo create_info;
  VkDeviceQueueCreateInfo queue_create_infos[1];
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.queueCreateInfoCount = sizeof(queue_create_infos) / sizeof(queue_create_infos[0]);

  queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_infos[0].pNext = NULL;
  queue_create_infos[0].flags = 0;
  queue_create_infos[0].queueFamilyIndex = lime_device.graphics_family_index;
  queue_create_infos[0].queueCount = 1;
  queue_create_infos[0].pQueuePriorities = &(float){1.0f};

  create_info.pQueueCreateInfos = queue_create_infos;
  /* TODO: Enable device layers (depricated) for compatibility. */
  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = NULL;
  create_info.enabledExtensionCount = sizeof(EXTENSIONS) / sizeof(EXTENSIONS[0]);
  create_info.ppEnabledExtensionNames = EXTENSIONS;
  create_info.pEnabledFeatures = NULL;

  assert(lime_device.device == VK_NULL_HANDLE);
  err = vkCreateDevice(physical_device, &create_info, NULL, &lime_device.device);
  ASSERT_VK_RESULT(err, "creating logical device");

  assert(lime_device.graphics_queue == VK_NULL_HANDLE);
  vkGetDeviceQueue(lime_device.device, lime_device.graphics_family_index, 0,
      &lime_device.graphics_queue);
}

void
lime_init_device(GLFWwindow *window)
{
  int validation_layers_enabled;
  VkResult err;
  validation_layers_enabled = check_validation_layer_support();
  if (!validation_layers_enabled)
    fprintf(stderr, "Validation layers not supported.\n");
  create_instance(validation_layers_enabled);
  if (validation_layers_enabled)
    create_debug_messenger();
  err = glfwCreateWindowSurface(instance, window, NULL, &lime_device.surface);
  ASSERT_VK_RESULT(err, "creating window surface");
  select_physical_device();

  /* TODO: check device settings supported. */
  lime_device.surface_format = VK_FORMAT_B8G8R8A8_UNORM;
  lime_device.present_mode = VK_PRESENT_MODE_FIFO_KHR;

  select_queue_family();
  create_device();
}

VkSurfaceCapabilitiesKHR
lime_get_current_surface_capabilities(void)
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, lime_device.surface, &surface_capabilities);
  return surface_capabilities;
}

uint32_t
lime_device_find_memory_type(uint32_t memory_type_bits, VkMemoryPropertyFlags properties)
{
  int i;
  for (i = 0; i < memory_properties.memoryTypeCount; i++)
    if (memory_type_bits & (1 << i)
        && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;
  fprintf(stderr, "Failed to find suitable memory type.\n");
  exit(1);
}

void
lime_destroy_device(void)
{
  PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy_func;

  vkDestroyDevice(lime_device.device, NULL);
  vkDestroySurfaceKHR(instance, lime_device.surface, NULL);
  if (debug_messenger != VK_NULL_HANDLE) {
    debug_messenger_destroy_func = (PFN_vkDestroyDebugUtilsMessengerEXT)
      vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT");
    debug_messenger_destroy_func(instance, debug_messenger, NULL);
  }
  vkDestroyInstance(instance, NULL);
}
