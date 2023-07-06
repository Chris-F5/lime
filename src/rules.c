#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

static int check_validation_layer_support(void);
static void noop(struct renderer *renderer, int rule);
static void dispatch_instance_rule(struct renderer *renderer, int rule);
static void destroy_instance_state(struct renderer *renderer, int rule);
static void dispatch_debug_messenger_rule(struct renderer *renderer, int rule);
static void destroy_debug_messenger_state(struct renderer *renderer, int rule);
static void dispatch_physical_device_rule(struct renderer *renderer, int rule);
static void dispatch_surface_rule(struct renderer *renderer, int rule);
static void destroy_surface_state(struct renderer *renderer, int rule);
static void dispatch_queue_family_rule(struct renderer *renderer, int rule);

static const char *VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";

void (*rule_dispatch_funcs[])(struct renderer *renderer, int rule) = {
  [RULE_TYPE_INSTANCE] = dispatch_instance_rule,
  [RULE_TYPE_DEBUG_MESSENGER] = dispatch_debug_messenger_rule,
  [RULE_TYPE_PHYSICAL_DEVICE] = dispatch_physical_device_rule,
  [RULE_TYPE_SURFACE] = dispatch_surface_rule,
  [RULE_TYPE_QUEUE_FAMILY] = dispatch_queue_family_rule,
};
void (*state_destroy_funcs[])(struct renderer *renderer, int rule) = {
  [RULE_TYPE_INSTANCE] = destroy_instance_state,
  [RULE_TYPE_DEBUG_MESSENGER] = destroy_debug_messenger_state,
  [RULE_TYPE_PHYSICAL_DEVICE] = noop,
  [RULE_TYPE_SURFACE] = destroy_surface_state,
  [RULE_TYPE_QUEUE_FAMILY] = noop,
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
noop(struct renderer *renderer, int rule)
{ }

int
add_instance_rule(struct renderer *renderer, int request_validation_layers)
{
  int rule;
  struct instance_conf *conf;
  rule = add_rule(renderer, RULE_TYPE_INSTANCE, sizeof(struct instance_conf),
      sizeof(struct instance_state));
  conf = get_rule_conf(renderer, rule);
  conf->validation_layers_requested = request_validation_layers;
  return rule;
}

static void
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
  state->validation_layers_enabled = conf->validation_layers_requested
    && check_validation_layer_support();
  if (conf->validation_layers_requested != state->validation_layers_enabled)
    fprintf(stderr, "validation layers requested but not available\n");

  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  if (state->validation_layers_enabled) {
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
  if (state->validation_layers_enabled) {
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

static void
destroy_instance_state(struct renderer *renderer, int rule)
{
  struct instance_state *state;
  state = get_rule_state(renderer, rule);
  vkDestroyInstance(state->instance, NULL);
}


int
add_debug_messenger_rule(struct renderer *renderer, int instance,
  VkDebugUtilsMessageSeverityFlagsEXT severity_flags,
  VkDebugUtilsMessageTypeFlagsEXT type_flags,
  PFN_vkDebugUtilsMessengerCallbackEXT callback_func)
{
  int rule;
  struct debug_messenger_conf *conf;
  rule = add_rule(renderer, RULE_TYPE_DEBUG_MESSENGER,
      sizeof(struct debug_messenger_conf), sizeof(struct debug_messenger_state));
  conf = get_rule_conf(renderer, rule);
  conf->severity_flags = severity_flags;
  conf->type_flags = type_flags;
  conf->callback_func = callback_func;
  add_rule_dependency(renderer, instance);
  return rule;
}

static void
dispatch_debug_messenger_rule(struct renderer *renderer, int rule)
{
  struct debug_messenger_conf *conf;
  struct debug_messenger_state *state;
  struct instance_state *instance;
  VkDebugUtilsMessengerCreateInfoEXT create_info;
  PFN_vkCreateDebugUtilsMessengerEXT create_func;
  VkResult err;

  conf = get_rule_conf(renderer, rule);
  state = get_rule_state(renderer, rule);
  instance = get_rule_dependency_state(renderer, rule, 0, RULE_TYPE_INSTANCE);
  if (!instance->validation_layers_enabled) {
    state->debug_messenger = VK_NULL_HANDLE;
    return;
  }

  create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.messageSeverity = conf->severity_flags;
  create_info.messageType = conf->type_flags;
  create_info.pfnUserCallback = conf->callback_func;
  create_info.pUserData = NULL;

  create_func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
    instance->instance, "vkCreateDebugUtilsMessengerEXT");
  assert(create_func);
  create_func(instance->instance, &create_info, NULL, &state->debug_messenger);
}

static void
destroy_debug_messenger_state(struct renderer *renderer, int rule)
{
  PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy_func;
  struct debug_messenger_state *state;
  struct instance_state *instance;

  state = get_rule_state(renderer, rule);
  if (state->debug_messenger == VK_NULL_HANDLE)
    return;
  instance = get_rule_dependency_state(renderer, rule, 0, RULE_TYPE_INSTANCE);
  debug_messenger_destroy_func = (PFN_vkDestroyDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(instance->instance, "vkDestroyDebugUtilsMessengerEXT");
  assert(debug_messenger_destroy_func);
  debug_messenger_destroy_func(instance->instance, state->debug_messenger, NULL);
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

static void
dispatch_physical_device_rule(struct renderer *renderer, int rule)
{
  struct physical_device_conf *conf;
  struct physical_device_state *state;
  struct instance_state *instance;
  uint32_t physical_device_count;
  VkPhysicalDevice *physical_devices;
  VkPhysicalDeviceProperties properties;
  int i;

  assert(renderer->rule_types[rule] == RULE_TYPE_PHYSICAL_DEVICE);
  conf = get_rule_conf(renderer, rule);
  state = get_rule_state(renderer, rule);
  instance = get_rule_dependency_state(renderer, rule, 0, RULE_TYPE_INSTANCE);

  vkEnumeratePhysicalDevices(instance->instance, &physical_device_count, NULL);
  physical_devices = xmalloc(physical_device_count * sizeof(VkPhysicalDevice));
  vkEnumeratePhysicalDevices(instance->instance, &physical_device_count,
      physical_devices);
  state->physical_device = VK_NULL_HANDLE;
  for (i = 0; i < physical_device_count; i++) {
    vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
    if (strcmp(conf->gpu_name, properties.deviceName) == 0) {
      state->physical_device = physical_devices[i];
      state->properties = properties;
      break;
    }
  }
  if (state->physical_device == VK_NULL_HANDLE) {
    fprintf(stderr, "no suitable GPU found\n");
    exit(1);
  }
}

int
add_window_surface_rule(struct renderer *renderer, int instance, GLFWwindow *window)
{
  int rule;
  struct surface_conf *conf;
  rule = add_rule(renderer, RULE_TYPE_SURFACE, sizeof(struct surface_conf),
      sizeof(struct surface_state));
  conf = get_rule_conf(renderer, rule);
  conf->window = window;
  add_rule_dependency(renderer, instance);
  return rule;
}

static void
dispatch_surface_rule(struct renderer *renderer, int rule)
{
  struct surface_conf *conf;
  struct surface_state *state;
  struct instance_state *instance;
  conf = get_rule_conf(renderer, rule);
  state = get_rule_state(renderer, rule);
  instance = get_rule_dependency_state(renderer, rule, 0, RULE_TYPE_INSTANCE);
  glfwCreateWindowSurface(instance->instance, conf->window, NULL, &state->surface);
}

static void
destroy_surface_state(struct renderer *renderer, int rule)
{
  struct surface_state *state;
  struct instance_state *instance;
  state = get_rule_state(renderer, rule);
  instance = get_rule_dependency_state(renderer, rule, 0, RULE_TYPE_INSTANCE);
  vkDestroySurfaceKHR(instance->instance, state->surface, NULL);
}

int
add_queue_family_rule(struct renderer *renderer, int physical_device,
    int surface, uint32_t required_flags)
{
  int rule;
  struct queue_family_conf *conf;
  rule = add_rule(renderer, RULE_TYPE_QUEUE_FAMILY, sizeof(struct queue_family_conf),
      sizeof(struct queue_family_state));
  conf = get_rule_conf(renderer, rule);
  conf->required_flags = required_flags;
  add_rule_dependency(renderer, physical_device);
  if (surface == -1) {
    conf->surface_support_required = 0;
  } else {
    conf->surface_support_required = 1;
    add_rule_dependency(renderer, surface);
  }
  return rule;
}

static void
dispatch_queue_family_rule(struct renderer *renderer, int rule)
{
  struct queue_family_conf *conf;
  struct queue_family_state *state;
  struct physical_device_state *physical;
  struct surface_state *surface;
  uint32_t family_count;
  VkQueueFamilyProperties *properties;
  uint32_t i;
  VkResult err;
  VkBool32 surface_support;

  conf = get_rule_conf(renderer, rule);
  state = get_rule_state(renderer, rule);
  physical = get_rule_dependency_state(renderer, rule, 0, RULE_TYPE_PHYSICAL_DEVICE);
  if (conf->surface_support_required)
    surface = get_rule_dependency_state(renderer, rule, 1, RULE_TYPE_SURFACE);
  vkGetPhysicalDeviceQueueFamilyProperties(physical->physical_device,
      &family_count, NULL);
  properties = xmalloc(family_count * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(physical->physical_device,
      &family_count, properties);
  for (i = 0; i < family_count; i++) {
    if (properties[i].queueFlags & conf->required_flags != conf->required_flags)
      continue;
    if (conf->surface_support_required) {
      err = vkGetPhysicalDeviceSurfaceSupportKHR(physical->physical_device, i,
          surface->surface, &surface_support);
      if (err != VK_SUCCESS) {
        PRINT_VK_ERROR(err, "checking queue family surface support");
        exit(1);
      }
      if (!surface_support)
        continue;
    }
    state->family_index = i;
    return;
  }
  fprintf(stderr, "no suitable queue family found\n");
  exit(1);
}
