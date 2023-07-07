#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL validation_layer_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);
static void configure_rules(struct renderer *renderer, GLFWwindow *window);
static void dispatch_rules(struct renderer *renderer);
static void destroy_state(struct renderer *renderer);
static int get_rule_dependency_count(const struct renderer *renderer, int rule);

static const char * const DEVICE_EXTENSIONS[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL
validation_layer_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    fprintf(stderr, "validation layer: %s\n", callback_data->pMessage);
    return VK_FALSE;
}

static void
configure_rules(struct renderer *renderer, GLFWwindow *window)
{
  int instance, physical_device, surface, surface_capabilities, graphics_family;
  int present_family, family_group, device, graphics_queue, present_queue;
  int swapchain, swapchain_images, graphics_command_pool;
  instance = add_instance_rule(renderer, 1);
  add_debug_messenger_rule(renderer, instance,
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      validation_layer_callback);
  physical_device = add_physical_device_rule(renderer, instance,
      "AMD Radeon RX 580 Series (RADV POLARIS10)");
  surface = add_window_surface_rule(renderer, instance, window);
  surface_capabilities = add_surface_capabilities_rule(renderer, physical_device,
      surface);
  graphics_family = add_queue_family_rule(renderer, physical_device, -1,
      VK_QUEUE_GRAPHICS_BIT);
  present_family = add_queue_family_rule(renderer, physical_device, surface, 0);
  family_group = add_queue_family_group_rule(renderer, 2,
      (int []){graphics_family, present_family});
  device = add_device_rule(renderer, physical_device, family_group,
      sizeof(DEVICE_EXTENSIONS[0]) / sizeof(DEVICE_EXTENSIONS),
      DEVICE_EXTENSIONS);
  graphics_queue = add_queue_rule(renderer, device, graphics_family);
  present_queue = add_queue_rule(renderer, device, present_family);
  swapchain = add_swapchain_rule(renderer, surface, surface_capabilities,
      device, family_group, 0, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      (VkSurfaceFormatKHR){VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_TRUE, VK_PRESENT_MODE_FIFO_KHR);
  swapchain_images = add_swapchain_image_views_rule(renderer, device, swapchain);
  graphics_command_pool = add_command_pool_rule(renderer, device, graphics_family,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

static void
dispatch_rules(struct renderer *renderer)
{
  int rule, type;
  void (*dispatch_func)(struct renderer *renderer, int rule);
  for (rule = 0; rule < renderer->rule_count; rule++) {
    type = renderer->rule_types[rule];
    dispatch_func = rule_dispatch_funcs[type];
    assert(dispatch_func);
    dispatch_func(renderer, rule);
  }
}

static void
destroy_state(struct renderer *renderer)
{
  int rule, type;
  void (*destroy_func)(struct renderer *renderer, int rule);
  for (rule = renderer->rule_count - 1; rule >= 0; rule--) {
    type = renderer->rule_types[rule];
    destroy_func = state_destroy_funcs[type];
    assert(destroy_func);
    destroy_func(renderer, rule);
  }
}

static int
get_rule_dependency_count(const struct renderer *renderer, int rule)
{
  assert(rule >= 0);
  assert(rule < renderer->rule_count);
  return (
      rule == renderer->rule_count - 1
      ? renderer->next_dependency_offset
      : renderer->dependency_offsets[rule + 1]
      ) - renderer->dependency_offsets[rule];
}

int
add_rule(struct renderer *renderer, int type, size_t conf_size, size_t state_size)
{
  int rule;
  rule = renderer->rule_count++;
  if (renderer->rule_count > renderer->rules_allocated) {
    renderer->rules_allocated += 64;
    renderer->rule_types = xrealloc(renderer->rule_types,
        renderer->rules_allocated * sizeof(int));
    renderer->conf_offsets = xrealloc(renderer->conf_offsets,
        renderer->rules_allocated * sizeof(long));
    renderer->state_offsets = xrealloc(renderer->state_offsets,
        renderer->rules_allocated * sizeof(long));
    renderer->dependency_offsets = xrealloc(renderer->dependency_offsets,
        renderer->rules_allocated * sizeof(int));
  }
  if (renderer->next_conf_offset + conf_size > renderer->conf_memory_allocated) {
    renderer->conf_memory_allocated
      = renderer->next_conf_offset + conf_size + 1024 * 4;
    renderer->conf_memory = xrealloc(renderer->conf_memory,
        renderer->conf_memory_allocated);
  }
  if (renderer->next_state_offset + state_size > renderer->state_memory_allocated) {
    renderer->state_memory_allocated
      = renderer->next_state_offset + state_size + 1024 * 4;
    renderer->state_memory = xrealloc(renderer->state_memory,
        renderer->state_memory_allocated);
  }
  renderer->rule_types[rule] = type;
  renderer->conf_offsets[rule] = renderer->next_conf_offset;
  renderer->next_conf_offset += conf_size;
  renderer->state_offsets[rule] = renderer->next_state_offset;
  renderer->next_state_offset += state_size;
  renderer->dependency_offsets[rule] = renderer->next_dependency_offset;
  return rule;
}

void *
get_rule_conf(const struct renderer *renderer, int rule)
{
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  return renderer->conf_memory + renderer->conf_offsets[rule];
}

void *
get_rule_state(const struct renderer *renderer, int rule)
{
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  return renderer->state_memory + renderer->state_offsets[rule];
}

void *
get_rule_dependency_state(const struct renderer *renderer, int rule,
    int dependency_index, int dependency_type)
{
  int dependency_rule;
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  if (dependency_index < 0
      || dependency_index > get_rule_dependency_count(renderer, rule)) {
    fprintf(stderr, "rule does not have dependency (%d)\n", dependency_index);
    exit(1);
  }
  dependency_rule = renderer->dependencies[
    renderer->dependency_offsets[rule] + dependency_index];
  if (renderer->rule_types[dependency_rule] != dependency_type) {
    fprintf(stderr, "unexpected dependency on rule type %d:\n\
  dependency index %d\n\
  expected type %d\n\
  actual type %d\n",
  renderer->rule_types[rule], dependency_index, dependency_type,
  renderer->rule_types[dependency_rule]);
    exit(1);
  }
  return renderer->state_memory + renderer->state_offsets[dependency_rule];
}

void
add_rule_dependency(struct renderer *renderer, int d)
{
  if (renderer->next_dependency_offset == renderer->dependencies_allocated) {
    renderer->dependencies_allocated += 128;
    renderer->dependencies = xrealloc(renderer->dependencies,
        renderer->dependencies_allocated * sizeof(int));
  }
  renderer->dependencies[renderer->next_dependency_offset++] = d;
}

void
create_renderer(struct renderer *renderer, GLFWwindow* window)
{
  renderer->rules_allocated = 0;
  renderer->rule_count = 0;
  renderer->conf_memory_allocated = 0;
  renderer->state_memory_allocated = 0;
  renderer->dependencies_allocated = 0;
  renderer->next_conf_offset = 0;
  renderer->next_state_offset = 0;
  renderer->next_dependency_offset = 0;
  renderer->rule_types = NULL;
  renderer->conf_offsets = NULL;
  renderer->state_offsets = NULL;
  renderer->dependency_offsets = NULL;
  renderer->conf_memory = NULL;
  renderer->state_memory = NULL;
  renderer->dependencies = NULL;
  configure_rules(renderer, window);
  dispatch_rules(renderer);
}

void
destroy_renderer(struct renderer *renderer)
{
  destroy_state(renderer);
  free(renderer->rule_types);
  free(renderer->conf_offsets);
  free(renderer->state_offsets);
  free(renderer->dependency_offsets);
  free(renderer->conf_memory);
  free(renderer->state_memory);
  free(renderer->dependencies);
}
