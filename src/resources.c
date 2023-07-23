#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "matrix.h"
#include "obj_types.h"
#include "lime.h"

static void create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities);
static void create_depth_image(void);
static void create_framebuffers(void);
static void allocate_buffers(void);
static void create_descriptor_pool(void);
static void allocate_descriptor_sets(void);
static void bind_descriptor_sets(void);

static VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
static VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
static VkImage depth_image;
static VkDeviceMemory depth_image_memory;
static VkImageView depth_image_view;
static int camera_uniform_buffer_step;
static VkBuffer camera_uniform_buffer;
static VkDeviceMemory camera_uniform_buffer_memory;
static VkDescriptorPool descriptor_pool;

struct lime_resources lime_resources;

static void
create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities)
{
  VkSwapchainCreateInfoKHR create_info;
  VkResult err;
  VkImageViewCreateInfo view_create_info;
  int i;
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.surface = lime_device.surface;
  if (surface_capabilities.minImageCount == surface_capabilities.maxImageCount)
    create_info.minImageCount = surface_capabilities.minImageCount;
  else
    create_info.minImageCount = surface_capabilities.minImageCount + 1;
  create_info.imageFormat = lime_device.surface_format.format;
  create_info.imageColorSpace = lime_device.surface_format.colorSpace;
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
  create_info.presentMode = lime_device.present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  lime_resources.swapchain_extent = create_info.imageExtent;
  err = vkCreateSwapchainKHR(lime_device.device, &create_info, NULL, &lime_resources.swapchain);
  ASSERT_VK_RESULT(err, "creating swapchain");

  err = vkGetSwapchainImagesKHR(lime_device.device, lime_resources.swapchain,
      &lime_resources.swapchain_image_count, NULL);
  ASSERT_VK_RESULT(err, "getting swapchain images");
  if (lime_resources.swapchain_image_count >= MAX_SWAPCHAIN_IMAGES) {
    fprintf(stderr, "Too many swapchain images.\n");
    exit(1);
  }
  err = vkGetSwapchainImagesKHR(lime_device.device, lime_resources.swapchain,
      &lime_resources.swapchain_image_count, swapchain_images);
  ASSERT_VK_RESULT(err, "getting swapchain images");

  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = NULL;
  view_create_info.flags = 0;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = lime_device.surface_format.format;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  for (i = 0; i < lime_resources.swapchain_image_count; i++) {
    view_create_info.image = swapchain_images[i];
    err = vkCreateImageView(lime_device.device, &view_create_info, NULL, &swapchain_image_views[i]);
    ASSERT_VK_RESULT(err, "creating swapchain image view");
  }
}

static void
create_depth_image(void)
{
  VkImageCreateInfo create_info;
  VkMemoryRequirements memory_requirements;
  VkMemoryAllocateInfo allocate_info;
  VkImageViewCreateInfo view_create_info;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.imageType = VK_IMAGE_TYPE_2D;
  create_info.format = lime_device.depth_format;
  create_info.extent.width = lime_resources.swapchain_extent.width;
  create_info.extent.height = lime_resources.swapchain_extent.height;
  create_info.extent.depth = 1;
  create_info.mipLevels = 1;
  create_info.arrayLayers = 1;
  create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  assert(depth_image == VK_NULL_HANDLE);
  err = vkCreateImage(lime_device.device, &create_info, NULL, &depth_image);
  ASSERT_VK_RESULT(err, "creating depth image");

  vkGetImageMemoryRequirements(lime_device.device, depth_image, &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  assert(depth_image_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL, &depth_image_memory);
  ASSERT_VK_RESULT(err, "allocating depth image memory");
  err = vkBindImageMemory(lime_device.device, depth_image, depth_image_memory, 0);
  ASSERT_VK_RESULT(err, "binding depth image memory");

  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = NULL;
  view_create_info.flags = 0;
  view_create_info.image = depth_image;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = lime_device.depth_format;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  assert(depth_image_view == VK_NULL_HANDLE);
  err = vkCreateImageView(lime_device.device, &view_create_info, NULL, &depth_image_view);
  ASSERT_VK_RESULT(err, "creating depth image view");
}

static void
create_framebuffers(void)
{
  VkImageView attachments[2];
  VkFramebufferCreateInfo create_info;
  int i;
  VkResult err;

  attachments[1] = depth_image_view;

  create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.renderPass = lime_pipelines.render_pass;
  create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
  create_info.pAttachments = attachments;
  create_info.width = lime_resources.swapchain_extent.width;
  create_info.height = lime_resources.swapchain_extent.height;
  create_info.layers = 1;
  for (i = 0; i < lime_resources.swapchain_image_count; i++) {
    attachments[0] = swapchain_image_views[i];
    assert(lime_resources.swapchain_framebuffers[i] == VK_NULL_HANDLE);
    err = vkCreateFramebuffer(lime_device.device, &create_info, NULL,
        &lime_resources.swapchain_framebuffers[i]);
    ASSERT_VK_RESULT(err, "creating swapcahin framebuffer");
  }
}

static void
allocate_buffers(void)
{
  VkBufferCreateInfo create_info;
  VkMemoryRequirements memory_requirements;
  VkMemoryAllocateInfo allocate_info;
  VkResult err;

  camera_uniform_buffer_step
    = (sizeof(struct camera_uniform_data) 
        / lime_device.properties.limits.minUniformBufferOffsetAlignment)
    * lime_device.properties.limits.minUniformBufferOffsetAlignment
    + lime_device.properties.limits.minUniformBufferOffsetAlignment;

  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = MAX_SWAPCHAIN_IMAGES * camera_uniform_buffer_step;
  create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(camera_uniform_buffer == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &camera_uniform_buffer);
  ASSERT_VK_RESULT(err, "creating camera uniform buffer");

  vkGetBufferMemoryRequirements(lime_device.device, camera_uniform_buffer,
      &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(camera_uniform_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL,
      &camera_uniform_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating camera uniform buffer memory");
  err = vkBindBufferMemory(lime_device.device, camera_uniform_buffer,
      camera_uniform_buffer_memory, 0);
}

static void
create_descriptor_pool(void)
{
  VkDescriptorPoolSize pool_sizes[1];
  VkDescriptorPoolCreateInfo create_info;
  VkResult err;

  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = MAX_SWAPCHAIN_IMAGES;
  create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.maxSets = MAX_SWAPCHAIN_IMAGES;
  create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
  create_info.pPoolSizes = pool_sizes;
  err = vkCreateDescriptorPool(lime_device.device, &create_info, NULL, &descriptor_pool);
  ASSERT_VK_RESULT(err, "creating descriptor pool");
}

static void
allocate_descriptor_sets(void)
{
  int i;
  VkDescriptorSetLayout layout_copies[MAX_SWAPCHAIN_IMAGES];
  VkDescriptorSetAllocateInfo allocate_info;
  VkResult err;
  for (i = 0; i < MAX_SWAPCHAIN_IMAGES; i++)
    layout_copies[i] = lime_pipelines.camera_descriptor_set_layout;
  allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.descriptorPool = descriptor_pool;
  allocate_info.descriptorSetCount = MAX_SWAPCHAIN_IMAGES;
  allocate_info.pSetLayouts = layout_copies;
  assert(lime_resources.camera_descriptor_sets[0] == VK_NULL_HANDLE);
  err = vkAllocateDescriptorSets(lime_device.device, &allocate_info,
      lime_resources.camera_descriptor_sets);
  ASSERT_VK_RESULT(err, "allocating descriptor sets");
}

static void
bind_descriptor_sets(void)
{
  int i;
  VkDescriptorBufferInfo buffer_info;
  VkWriteDescriptorSet write;
  buffer_info.buffer = camera_uniform_buffer;
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = NULL;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write.pImageInfo = NULL;
  write.pBufferInfo = &buffer_info;
  write.pTexelBufferView = NULL;
  for (i = 0; i < MAX_SWAPCHAIN_IMAGES; i++) {
    buffer_info.offset = i * camera_uniform_buffer_step;
    buffer_info.range = sizeof(struct camera_uniform_data);
    write.dstSet = lime_resources.camera_descriptor_sets[i];
    vkUpdateDescriptorSets(lime_device.device, 1, &write, 0, NULL);
  }
}

void
lime_init_resources(void)
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  surface_capabilities = lime_get_current_surface_capabilities();
  create_swapchain(surface_capabilities);
  create_depth_image();
  create_framebuffers();
  allocate_buffers();
  create_descriptor_pool();
  allocate_descriptor_sets();
  bind_descriptor_sets();
}

void
set_camera_uniform_data(int swap_index, struct camera_uniform_data data)
{
  struct camera_uniform_data *mapped;
  VkResult err;
  err = vkMapMemory(lime_device.device, camera_uniform_buffer_memory,
      swap_index * camera_uniform_buffer_step, sizeof(struct camera_uniform_data),
      0, (void **)&mapped);
  ASSERT_VK_RESULT(err, "mapping camera uniform buffer data");
  *mapped = data;
  vkUnmapMemory(lime_device.device, camera_uniform_buffer_memory);
}

void
lime_destroy_resources(void)
{
  int i;
  vkDestroyDescriptorPool(lime_device.device, descriptor_pool, NULL);
  vkDestroyBuffer(lime_device.device, camera_uniform_buffer, NULL);
  vkFreeMemory(lime_device.device, camera_uniform_buffer_memory, NULL);
  vkDestroyImageView(lime_device.device, depth_image_view, NULL);
  vkDestroyImage(lime_device.device, depth_image, NULL);
  vkFreeMemory(lime_device.device, depth_image_memory, NULL);
  for (i = 0; i < lime_resources.swapchain_image_count; i++) {
    vkDestroyFramebuffer(lime_device.device, lime_resources.swapchain_framebuffers[i], NULL);
    vkDestroyImageView(lime_device.device, swapchain_image_views[i], NULL);
  }
  vkDestroySwapchainKHR(lime_device.device, lime_resources.swapchain, NULL);
}
