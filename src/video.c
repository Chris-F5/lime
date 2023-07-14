#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"
#include "utils.h"

static int check_validation_layer_support(void);
static void init_instance(int validation_layers_enabled);
static VKAPI_ATTR VkBool32 VKAPI_CALL validation_layer_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);
static void create_debug_messenger(void);
static int check_physical_device_extension_support(VkPhysicalDevice physical_device);
static void select_physical_device(void);

static const char *VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
static const char * const EXTENSIONS[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkInstance instance;
static VkDebugUtilsMessengerEXT debug_messenger;
static VkSurfaceKHR surface;
static VkPhysicalDevice physical_device;
static VkPhysicalDeviceProperties physical_device_properties;
static VkSurfaceFormatKHR surfaceFormat;
static VkPresentModeKHR presentMode;

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
init_instance(int validation_layers_enabled)
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

  assert(instance == VK_NULL_HANDLE);
  err = vkCreateInstance(&create_info, NULL, &instance);
  ASSERT_VK_RESULT(err, "creating instance");

  if (extensions != glfw_extensions)
    free(extensions);
}

static void
glfw_error_callback(int _, const char* str)
{
    printf("glfw error: '%s'\n", str);
    exit(1);
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
  int available_extension_count, r, a;
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
  int i;
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
  if (!check_physical_device_extension_support(physical_device)) {
    fprintf(stderr, "Physical device does not support required extensions.\n");
    exit(1);
  }
  free(physical_devices);
}

void
init_video(GLFWwindow *window)
{
  int validation_layers_enabled;
  VkResult err;
  validation_layers_enabled = check_validation_layer_support();
  if (!validation_layers_enabled)
    fprintf(stderr, "Validation layers not supported.\n");
  init_instance(validation_layers_enabled);
  if (validation_layers_enabled)
    create_debug_messenger();
  err = glfwCreateWindowSurface(instance, window, NULL, &surface);
  ASSERT_VK_RESULT(err, "creating window surface");
  select_physical_device();
}

void
destroy_video(void)
{
  PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy_func;
  vkDestroySurfaceKHR(instance, surface, NULL);
  if (debug_messenger != VK_NULL_HANDLE) {
    debug_messenger_destroy_func = (PFN_vkDestroyDebugUtilsMessengerEXT)
      vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT");
    debug_messenger_destroy_func(instance, debug_messenger, NULL);
  }
  vkDestroyInstance(instance, NULL);
}
