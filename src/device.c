#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

static const char *VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

static int check_validation_layer_support(void);
/*
static VKAPI_ATTR VkBool32 VKAPI_CALL validation_layer_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);
static void create_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT *debug_messenger);
*/

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

/*
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
*/

int
add_instance_rule(struct renderer *renderer)
{
  int rule;
  struct instance_conf *conf;
  rule = add_rule(renderer, RULE_TYPE_INSTANCE, sizeof(struct instance_conf),
      sizeof(struct instance_state));
  conf = get_rule_conf(renderer, rule);
  conf->validation_layers_enabled = check_validation_layer_support();
  return rule;
}

void
dispatch_instance_rule(struct renderer *renderer, int rule)
{
  struct instance_conf *conf;
  struct instance_state *state;
  int glfw_extension_count, extension_count;
  const char **glfw_extensions, **extensions;
  VkApplicationInfo app_info;
  VkInstanceCreateInfo create_info;
  VkResult err;

  assert(renderer->rule_types[rule] == RULE_TYPE_INSTANCE);
  conf = get_rule_conf(renderer, rule);
  state = get_rule_state(renderer, rule);

  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  if (conf->validation_layers_enabled) {
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
  if (conf->validation_layers_enabled) {
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = &VALIDATION_LAYER;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;
  }
  err = vkCreateInstance(&create_info, NULL, &state->instance);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "creating instance");
    exit(1);
  }
  if (extensions != glfw_extensions)
    free(extensions);
}

void
destroy_instance_state(struct renderer *renderer, int rule)
{
  struct instance_state *state;
  state = get_rule_state(renderer, rule);
  vkDestroyInstance(state->instance, NULL);
}

int
add_physical_device_rule(struct renderer *renderer, int instance, const char *gpu_name)
{
  int rule;
  struct physical_device_conf *conf;
  rule = add_rule(renderer, RULE_TYPE_PHYSICAL_DEVICE,
      sizeof(struct physical_device_conf), sizeof(struct physical_device_state));
  conf = get_rule_conf(renderer, rule);
  conf->gpu_name = gpu_name;
  add_rule_dependency(renderer, instance);
  return rule;
}

void
dispatch_physical_device_rule(struct renderer *renderer, int rule)
{
  struct physical_device_conf *conf;
  struct physical_device_state *state;
  struct instance_state *instance;
  assert(renderer->rule_types[rule] == RULE_TYPE_PHYSICAL_DEVICE);
  conf = get_rule_conf(renderer, rule);
  state = get_rule_state(renderer, rule);
  instance = get_rule_dependency_state(renderer, rule, 0, RULE_TYPE_INSTANCE);
}

void
destroy_physical_device_state(struct renderer *renderer, int rule)
{
  struct physical_device_state *state;
  state = get_rule_state(renderer, rule);
}
