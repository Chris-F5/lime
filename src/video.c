#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"
#include "utils.h"

#define MAX_SWAPCHAIN_IMAGES 8

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
static void select_queue_family(void);
static void init_device(void);
static void create_swapchain(void);
static void init_render_passes(void);
static void init_framebuffers(void);
static void init_synchronization_objects(void);
static void create_graphics_command_pool(void);
static void allocate_command_buffers(void);
static void record_command_buffer(VkCommandBuffer command_buffer, int swap_index);

static const char *VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
static const char * const EXTENSIONS[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkInstance instance;
static VkDebugUtilsMessengerEXT debug_messenger;
static VkSurfaceKHR surface;
static VkSurfaceCapabilitiesKHR surface_capabilities;
static VkPhysicalDevice physical_device;
static VkPhysicalDeviceProperties physical_device_properties;
static VkPresentModeKHR present_mode;
static VkSwapchainKHR swapchain;
static uint32_t swapchain_image_count;
static VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
static VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
static VkFramebuffer swapchain_framebuffers[MAX_SWAPCHAIN_IMAGES];
static VkCommandPool graphics_command_pool;
static VkCommandBuffer command_buffers[MAX_SWAPCHAIN_IMAGES];
static VkSemaphore image_available_semaphore, render_finished_semaphore;
static VkFence frame_finished_fence;

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

  for (i = 0; i < count; i++) {
    err = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &surface_support);
    ASSERT_VK_RESULT(err, "getting queue family surface support");
    if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && surface_support) {
      vk_globals.graphics_family_index = i;
      break;
    }
  }
  free(properties);
}

static void
init_device(void)
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
  queue_create_infos[0].queueFamilyIndex = vk_globals.graphics_family_index;
  queue_create_infos[0].queueCount = 1;
  queue_create_infos[0].pQueuePriorities = &(float){1.0f};

  create_info.pQueueCreateInfos = queue_create_infos;
  /* TODO: Enable device layers (depricated) for compatibility. */
  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = NULL;
  create_info.enabledExtensionCount = sizeof(EXTENSIONS) / sizeof(EXTENSIONS[0]);
  create_info.ppEnabledExtensionNames = EXTENSIONS;
  create_info.pEnabledFeatures = NULL;

  assert(vk_globals.device == VK_NULL_HANDLE);
  err = vkCreateDevice(physical_device, &create_info, NULL, &vk_globals.device);
  ASSERT_VK_RESULT(err, "creating logical device");

  assert(vk_globals.graphics_queue == VK_NULL_HANDLE);
  vkGetDeviceQueue(vk_globals.device, vk_globals.graphics_family_index, 0,
      &vk_globals.graphics_queue);
}

static void
create_swapchain(void)
{
  VkSwapchainCreateInfoKHR create_info;
  VkResult err;
  VkImageViewCreateInfo view_create_info;
  int i;
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.surface = surface;
  if (surface_capabilities.minImageCount == surface_capabilities.maxImageCount)
    create_info.minImageCount = surface_capabilities.minImageCount;
  else
    create_info.minImageCount = surface_capabilities.minImageCount + 1;
  create_info.imageFormat = vk_globals.surface_format.format;
  create_info.imageColorSpace = vk_globals.surface_format.colorSpace;
  if (surface_capabilities.currentExtent.width == 0xffffffff) {
    create_info.imageExtent.width = 500;
    create_info.imageExtent.height = 500;
    fprintf(stderr, "surface did not specify extent\n");
  } else {
    create_info.imageExtent = surface_capabilities.currentExtent;
  }
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  create_info.preTransform = surface_capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  vk_globals.swapchain_extent = create_info.imageExtent;
  err = vkCreateSwapchainKHR(vk_globals.device, &create_info, NULL, &swapchain);
  ASSERT_VK_RESULT(err, "creating swapchain");

  err = vkGetSwapchainImagesKHR(vk_globals.device, swapchain, &swapchain_image_count, NULL);
  ASSERT_VK_RESULT(err, "getting swapchain images");
  if (swapchain_image_count >= MAX_SWAPCHAIN_IMAGES) {
    fprintf(stderr, "Too many swapchain images.\n");
    exit(1);
  }
  err = vkGetSwapchainImagesKHR(vk_globals.device, swapchain, &swapchain_image_count, swapchain_images);
  ASSERT_VK_RESULT(err, "getting swapchain images");

  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = NULL;
  view_create_info.flags = 0;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = vk_globals.surface_format.format;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  for (i = 0; i < swapchain_image_count; i++) {
    view_create_info.image = swapchain_images[i];
    err = vkCreateImageView(vk_globals.device, &view_create_info, NULL, &swapchain_image_views[i]);
    ASSERT_VK_RESULT(err, "creating swapchain image view");
  }
}

static void
init_render_passes(void)
{
  VkAttachmentDescription attachments[1];
  VkAttachmentReference color_attachments[1];
  VkSubpassDescription subpass;
  VkRenderPassCreateInfo create_info;
  VkResult err;

  attachments[0].flags = 0;
  attachments[0].format = vk_globals.surface_format.format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  color_attachments[0].attachment = 0;
  color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = NULL;
  subpass.colorAttachmentCount = sizeof(color_attachments) / sizeof(color_attachments[0]);
  subpass.pColorAttachments = color_attachments;
  subpass.pResolveAttachments = NULL;
  subpass.pDepthStencilAttachment = NULL;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = NULL;

  create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
  create_info.pAttachments = attachments;
  create_info.subpassCount = 1;
  create_info.pSubpasses = &subpass;
  create_info.dependencyCount = 0;
  create_info.pDependencies = NULL;
  assert(vk_globals.render_pass == VK_NULL_HANDLE);
  err = vkCreateRenderPass(vk_globals.device, &create_info, NULL, &vk_globals.render_pass);
  ASSERT_VK_RESULT(err, "creating render pass");
}

static void
init_framebuffers(void)
{
  VkFramebufferCreateInfo create_info;
  int i;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.renderPass = vk_globals.render_pass;
  create_info.attachmentCount = 1;
  create_info.width = vk_globals.swapchain_extent.width;
  create_info.height = vk_globals.swapchain_extent.height;
  create_info.layers = 1;
  for (i = 0; i < swapchain_image_count; i++) {
    create_info.pAttachments = &swapchain_image_views[i];
    assert(swapchain_framebuffers[i] == VK_NULL_HANDLE);
    err = vkCreateFramebuffer(vk_globals.device, &create_info, NULL, &swapchain_framebuffers[i]);
    ASSERT_VK_RESULT(err, "creating swapcahin framebuffer");
  }
}

static void
init_synchronization_objects(void)
{
  VkSemaphoreCreateInfo semaphore_create_info;
  VkFenceCreateInfo fence_create_info;
  VkResult err;
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_create_info.pNext = NULL;
  semaphore_create_info.flags = 0;
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.pNext = NULL;
  fence_create_info.flags = 0;
  assert(image_available_semaphore == VK_NULL_HANDLE);
  assert(render_finished_semaphore == VK_NULL_HANDLE);
  assert(frame_finished_fence == VK_NULL_HANDLE);
  err = vkCreateSemaphore(vk_globals.device, &semaphore_create_info, NULL,
      &image_available_semaphore);
  ASSERT_VK_RESULT(err, "creating semaphore");
  err = vkCreateSemaphore(vk_globals.device, &semaphore_create_info, NULL,
      &render_finished_semaphore);
  ASSERT_VK_RESULT(err, "creating semaphore");
  err = vkCreateFence(vk_globals.device, &fence_create_info, NULL,
      &frame_finished_fence);
}

static void
create_graphics_command_pool(void)
{
  VkCommandPoolCreateInfo create_info;
  VkResult err;
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  create_info.queueFamilyIndex = vk_globals.graphics_family_index;
  assert(graphics_command_pool == VK_NULL_HANDLE);
  err = vkCreateCommandPool(vk_globals.device, &create_info, NULL, &graphics_command_pool);
  ASSERT_VK_RESULT(err, "creating graphics command pool");
}

static void
allocate_command_buffers(void)
{
  VkCommandBufferAllocateInfo allocate_info;
  VkResult err;
  allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.commandPool = graphics_command_pool;
  allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocate_info.commandBufferCount = swapchain_image_count;
  assert(command_buffers[0] == VK_NULL_HANDLE);
  err = vkAllocateCommandBuffers(vk_globals.device, &allocate_info, command_buffers);
  ASSERT_VK_RESULT(err, "allocating command buffers");
}

static void
record_command_buffer(VkCommandBuffer command_buffer, int swap_index)
{
  VkCommandBufferBeginInfo begin_info;
  VkClearValue clear_value;
  VkRenderPassBeginInfo render_pass_info;
  VkResult err;
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = NULL;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = NULL;
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  ASSERT_VK_RESULT(err, "begining command buffer");

  assert(vk_globals.surface_format.format == VK_FORMAT_B8G8R8A8_UNORM);
  clear_value.color.float32[0] = 128.0f / 255.0f;
  clear_value.color.float32[1] = 218.0f / 255.0f;
  clear_value.color.float32[2] = 251.0f / 255.0f;
  clear_value.color.float32[3] = 1.0f;
  clear_value.depthStencil.depth = 0.0f;
  clear_value.depthStencil.stencil = 0;

  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.pNext = NULL;
  render_pass_info.renderPass = vk_globals.render_pass;
  render_pass_info.framebuffer = swapchain_framebuffers[swap_index];
  render_pass_info.renderArea.offset.x = 0;
  render_pass_info.renderArea.offset.y = 0;
  render_pass_info.renderArea.extent = vk_globals.swapchain_extent;
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_value;
  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_globals.pipeline);
  vkCmdDraw(command_buffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(command_buffer);
  err = vkEndCommandBuffer(command_buffer);
  ASSERT_VK_RESULT(err, "recording command buffer");
}

void
record_command_buffers(void)
{
  int i;
  for (i = 0; i < swapchain_image_count; i++)
    record_command_buffer(command_buffers[i], i);
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
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
  select_queue_family();
  init_device();
  /* TODO: check surface format and present mode are supported. */
  vk_globals.surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
  vk_globals.surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  present_mode = VK_PRESENT_MODE_FIFO_KHR;
  create_swapchain();
  init_render_passes();
  init_framebuffers();
  create_graphics_command_pool();
  allocate_command_buffers();
  init_synchronization_objects();
}

void
draw_frame(void)
{
}

void
destroy_video(void)
{
  PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destroy_func;
  int i;

  vkDestroySemaphore(vk_globals.device, image_available_semaphore, NULL);
  vkDestroySemaphore(vk_globals.device, render_finished_semaphore, NULL);
  vkDestroyFence(vk_globals.device, frame_finished_fence, NULL);

  vkDestroyCommandPool(vk_globals.device, graphics_command_pool, NULL);

  for (i = 0; i < swapchain_image_count; i++)
    vkDestroyFramebuffer(vk_globals.device, swapchain_framebuffers[i], NULL);

  vkDestroyRenderPass(vk_globals.device, vk_globals.render_pass, NULL);
  for (i = 0; i < swapchain_image_count; i++)
    vkDestroyImageView(vk_globals.device, swapchain_image_views[i], NULL);
  vkDestroySwapchainKHR(vk_globals.device, swapchain, NULL);

  vkDestroyDevice(vk_globals.device, NULL);
  vkDestroySurfaceKHR(instance, surface, NULL);
  if (debug_messenger != VK_NULL_HANDLE) {
    debug_messenger_destroy_func = (PFN_vkDestroyDebugUtilsMessengerEXT)
      vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT");
    debug_messenger_destroy_func(instance, debug_messenger, NULL);
  }
  vkDestroyInstance(instance, NULL);
}
