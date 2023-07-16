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

static void create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities);
static void create_framebuffers(void);
static void create_synchronization_objects(void);
static void create_graphics_command_pool(void);
static void allocate_command_buffers(void);
static void record_command_buffer(VkCommandBuffer command_buffer, int swap_index);
static void record_command_buffers(void);

static VkSwapchainKHR swapchain;
static uint32_t swapchain_image_count;
static VkExtent2D swapchain_extent;
static VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
static VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];
static VkFramebuffer swapchain_framebuffers[MAX_SWAPCHAIN_IMAGES];
static VkCommandPool graphics_command_pool;
static VkCommandBuffer command_buffers[MAX_SWAPCHAIN_IMAGES];
static VkSemaphore image_available_semaphore, render_finished_semaphore;
static VkFence frame_finished_fence;

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
  create_info.imageFormat = lime_device.surface_format;
  create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
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

  swapchain_extent = create_info.imageExtent;
  err = vkCreateSwapchainKHR(lime_device.device, &create_info, NULL, &swapchain);
  ASSERT_VK_RESULT(err, "creating swapchain");

  err = vkGetSwapchainImagesKHR(lime_device.device, swapchain, &swapchain_image_count, NULL);
  ASSERT_VK_RESULT(err, "getting swapchain images");
  if (swapchain_image_count >= MAX_SWAPCHAIN_IMAGES) {
    fprintf(stderr, "Too many swapchain images.\n");
    exit(1);
  }
  err = vkGetSwapchainImagesKHR(lime_device.device, swapchain, &swapchain_image_count, swapchain_images);
  ASSERT_VK_RESULT(err, "getting swapchain images");

  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = NULL;
  view_create_info.flags = 0;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = lime_device.surface_format;
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
    err = vkCreateImageView(lime_device.device, &view_create_info, NULL, &swapchain_image_views[i]);
    ASSERT_VK_RESULT(err, "creating swapchain image view");
  }
}

static void
create_framebuffers(void)
{
  VkFramebufferCreateInfo create_info;
  int i;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.renderPass = lime_pipelines.render_pass;
  create_info.attachmentCount = 1;
  create_info.width = swapchain_extent.width;
  create_info.height = swapchain_extent.height;
  create_info.layers = 1;
  for (i = 0; i < swapchain_image_count; i++) {
    create_info.pAttachments = &swapchain_image_views[i];
    assert(swapchain_framebuffers[i] == VK_NULL_HANDLE);
    err = vkCreateFramebuffer(lime_device.device, &create_info, NULL, &swapchain_framebuffers[i]);
    ASSERT_VK_RESULT(err, "creating swapcahin framebuffer");
  }
}

static void
create_synchronization_objects(void)
{
  VkSemaphoreCreateInfo semaphore_create_info;
  VkFenceCreateInfo fence_create_info;
  VkResult err;
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_create_info.pNext = NULL;
  semaphore_create_info.flags = 0;
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.pNext = NULL;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  assert(image_available_semaphore == VK_NULL_HANDLE);
  assert(render_finished_semaphore == VK_NULL_HANDLE);
  assert(frame_finished_fence == VK_NULL_HANDLE);
  err = vkCreateSemaphore(lime_device.device, &semaphore_create_info, NULL,
      &image_available_semaphore);
  ASSERT_VK_RESULT(err, "creating semaphore");
  err = vkCreateSemaphore(lime_device.device, &semaphore_create_info, NULL,
      &render_finished_semaphore);
  ASSERT_VK_RESULT(err, "creating semaphore");
  err = vkCreateFence(lime_device.device, &fence_create_info, NULL,
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
  create_info.queueFamilyIndex = lime_device.graphics_family_index;
  assert(graphics_command_pool == VK_NULL_HANDLE);
  err = vkCreateCommandPool(lime_device.device, &create_info, NULL, &graphics_command_pool);
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
  err = vkAllocateCommandBuffers(lime_device.device, &allocate_info, command_buffers);
  ASSERT_VK_RESULT(err, "allocating command buffers");
}

static void
record_command_buffer(VkCommandBuffer command_buffer, int swap_index)
{
  VkCommandBufferBeginInfo begin_info;
  VkClearValue clear_value;
  VkRenderPassBeginInfo render_pass_info;
  VkViewport viewport;
  VkRect2D scissor;
  VkResult err;

  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = NULL;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = NULL;
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  ASSERT_VK_RESULT(err, "begining command buffer");

  assert(lime_device.surface_format == VK_FORMAT_B8G8R8A8_UNORM);
  clear_value.color.float32[0] = 128.0f / 255.0;
  clear_value.color.float32[1] = 218.0f / 255.0;
  clear_value.color.float32[2] = 251.0f / 255.0;
  clear_value.color.float32[3] = 1.0f;

  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.pNext = NULL;
  render_pass_info.renderPass = lime_pipelines.render_pass;
  render_pass_info.framebuffer = swapchain_framebuffers[swap_index];
  render_pass_info.renderArea.offset.x = 0;
  render_pass_info.renderArea.offset.y = 0;
  render_pass_info.renderArea.extent = swapchain_extent;
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_value;

  viewport.x = viewport.y = 0.0f;
  viewport.width = swapchain_extent.width;
  viewport.height = swapchain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent = swapchain_extent;

  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lime_pipelines.pipeline);
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  vkCmdDraw(command_buffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(command_buffer);
  err = vkEndCommandBuffer(command_buffer);
  ASSERT_VK_RESULT(err, "recording command buffer");
}

static void
record_command_buffers(void)
{
  int i;
  for (i = 0; i < swapchain_image_count; i++)
    record_command_buffer(command_buffers[i], i);
}

void
lime_init_renderer(void)
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  surface_capabilities = lime_get_current_surface_capabilities();
  create_swapchain(surface_capabilities);
  create_framebuffers();
  create_graphics_command_pool();
  allocate_command_buffers();
  record_command_buffers();
  create_synchronization_objects();
}

void
lime_draw_frame(void)
{
  uint32_t swapchain_index;
  VkSubmitInfo submit_info;
  VkPresentInfoKHR present_info;
  VkResult err;

  vkWaitForFences(lime_device.device, 1, &frame_finished_fence, VK_TRUE, UINT64_MAX);
  vkResetFences(lime_device.device, 1, &frame_finished_fence);
  err = vkAcquireNextImageKHR(lime_device.device, swapchain, UINT64_MAX,
      image_available_semaphore, VK_NULL_HANDLE, &swapchain_index);
  ASSERT_VK_RESULT(err, "acquiring next swapchain image");
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = NULL;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &image_available_semaphore;
  submit_info.pWaitDstStageMask = &(VkPipelineStageFlags){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffers[swapchain_index];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &render_finished_semaphore;
  err = vkQueueSubmit(lime_device.graphics_queue, 1, &submit_info, frame_finished_fence);
  ASSERT_VK_RESULT(err, "submitting command buffer");
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = NULL;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &render_finished_semaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain;
  present_info.pImageIndices = &swapchain_index;
  present_info.pResults = NULL;
  err = vkQueuePresentKHR(lime_device.graphics_queue, &present_info);
  ASSERT_VK_RESULT(err, "submitting present request");
}

void
lime_destroy_renderer(void)
{
  int i;
  vkDestroySemaphore(lime_device.device, image_available_semaphore, NULL);
  vkDestroySemaphore(lime_device.device, render_finished_semaphore, NULL);
  vkDestroyFence(lime_device.device, frame_finished_fence, NULL);
  vkDestroyCommandPool(lime_device.device, graphics_command_pool, NULL);
  for (i = 0; i < swapchain_image_count; i++)
    vkDestroyFramebuffer(lime_device.device, swapchain_framebuffers[i], NULL);
  for (i = 0; i < swapchain_image_count; i++)
    vkDestroyImageView(lime_device.device, swapchain_image_views[i], NULL);
  vkDestroySwapchainKHR(lime_device.device, swapchain, NULL);
}
