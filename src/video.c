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

static const char *VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

static VkInstance instance;
static VkDebugUtilsMessengerEXT debug_messenger;
static VkSurfaceKHR surface;
static VkSurfaceFormatKHR surfaceFormat;
static VkPresentModeKHR presentMode;
static VkFormat depthImageFormat;

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

void
init_video(void)
{
  int validation_layers_enabled;
  validation_layers_enabled = check_validation_layer_support();
  if (!validation_layers_enabled)
    fprintf(stderr, "Validation layers not supported.\n");
  init_instance(validation_layers_enabled);
  if (validation_layers_enabled)
    create_debug_messenger();
}

void
destroy_video(void)
{
  PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy_func;
  if (debug_messenger != VK_NULL_HANDLE) {
    debug_messenger_destroy_func = (PFN_vkDestroyDebugUtilsMessengerEXT)
      vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT");
    debug_messenger_destroy_func(instance, debug_messenger, NULL);
  }
  vkDestroyInstance(instance, NULL);
}
