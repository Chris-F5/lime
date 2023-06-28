#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

static const char *VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
static const char * const DEVICE_EXTENSIONS[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

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
create_vulkan_instance(VkInstance *instance, int validation_layers_enabled)
{
  int glfw_extension_count, extension_count;
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

  err = vkCreateInstance(&create_info, NULL, instance);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "creating instance");
    exit(1);
  }

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

void
create_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *debug_messenger)
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
  if (create_func == NULL) {
    PRINT_VK_ERROR(VK_ERROR_EXTENSION_NOT_PRESENT, "creating debug messenger");
    exit(1);
  }
  create_func(instance, &create_info, NULL, debug_messenger);
}

void
create_renderer(struct lime_renderer *renderer, GLFWwindow* window)
{
  VkResult err;
  VkPhysicalDevice physical_device;
  int graphics_queue_family_index, present_queue_family_index;
  int queue_families[2];
  VkPhysicalDeviceFeatures device_features;
  VkDevice logical_device;
  VkCommandPool graphics_pool;

  renderer->validation_layers_enabled = check_validation_layer_support();
  if(!renderer->validation_layers_enabled)
    fprintf(stderr, "validation layers are not supported\n");
  create_vulkan_instance(&renderer->instance, renderer->validation_layers_enabled);
  if (renderer->validation_layers_enabled)
    create_debug_messenger(renderer->instance, &renderer->debug_messenger);
  else
    renderer->debug_messenger = VK_NULL_HANDLE;
  err = glfwCreateWindowSurface(renderer->instance, window, NULL, &renderer->surface);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "creating window surface");
    exit(1);
  }
  renderer->surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
  renderer->surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  renderer->present_mode = VK_PRESENT_MODE_FIFO_KHR;
  renderer->depth_image_format = VK_FORMAT_D32_SFLOAT;

  create_physical_device_table(&renderer->physical_devices, renderer->instance,
      renderer->surface);
  create_queue_family_table(&renderer->queue_families,
      &renderer->physical_devices, renderer->instance, renderer->surface);
  create_logical_device_table(&renderer->logical_devices);
  create_queue_table(&renderer->device_queues);
  create_command_pool_table(&renderer->command_pools);

  if (renderer->physical_devices.count == 0) {
    fprintf(stderr, "no graphics cards with vulkan support found\n");
    exit(1);
  }
  physical_device = renderer->physical_devices.physical_device[0];

  if(!check_physical_device_extension_support(physical_device,
        sizeof(DEVICE_EXTENSIONS[0]) / sizeof(DEVICE_EXTENSIONS),
        DEVICE_EXTENSIONS)) {
    fprintf(stderr, "graphics card does not have required extension support\n");
    exit(1);
  }

  graphics_queue_family_index = select_queue_family_with_flags(
      &renderer->queue_families, physical_device, VK_QUEUE_GRAPHICS_BIT);
  present_queue_family_index = select_queue_family_with_present_support(
      &renderer->queue_families, physical_device, renderer->surface);
  queue_families[0] = graphics_queue_family_index;
  queue_families[1] = present_queue_family_index;
  printf("graphics queue family index: %d\n", graphics_queue_family_index);
  printf("present queue family index: %d\n", present_queue_family_index);

  memset(&device_features, 0, sizeof(VkPhysicalDeviceFeatures));
  logical_device = create_logical_device(&renderer->logical_devices,
      &renderer->device_queues, physical_device,
      queue_families[0] == queue_families[1] ? 1 : 2, queue_families,
      sizeof(DEVICE_EXTENSIONS[0]) / sizeof(DEVICE_EXTENSIONS),
      DEVICE_EXTENSIONS, device_features);
  printf("queue count: %d\n", renderer->device_queues.count);

  create_command_pool(&renderer->command_pools, logical_device,
      graphics_queue_family_index,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

void
destroy_renderer(struct lime_renderer *renderer)
{
  PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy_func;
  destroy_command_pool_table(&renderer->command_pools);
  destroy_queue_table(&renderer->device_queues);
  destroy_logical_device_table(&renderer->logical_devices);
  destroy_queue_family_table(&renderer->queue_families);
  destroy_physical_device_table(&renderer->physical_devices);
  vkDestroySurfaceKHR(renderer->instance, renderer->surface, NULL);
  debug_messenger_destroy_func = (PFN_vkDestroyDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(renderer->instance,"vkDestroyDebugUtilsMessengerEXT");
  if (debug_messenger_destroy_func != NULL)
    debug_messenger_destroy_func(renderer->instance, renderer->debug_messenger,
        NULL);
  vkDestroyInstance(renderer->instance, NULL);
}
