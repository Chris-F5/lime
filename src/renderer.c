#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "obj_types.h"
#include "lime.h"
#include "utils.h"

static void create_synchronization_objects(void);
static void create_graphics_command_pool(void);
static void allocate_command_buffers(void);
static void record_command_buffer(VkCommandBuffer command_buffer, int swap_index);
static void record_command_buffers(void);

static VkCommandPool graphics_command_pool;
static VkCommandBuffer command_buffers[MAX_SWAPCHAIN_IMAGES];
static VkSemaphore image_available_semaphore, render_finished_semaphore;
static VkFence frame_finished_fence;

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
  allocate_info.commandBufferCount = lime_resources.swapchain_image_count;
  assert(command_buffers[0] == VK_NULL_HANDLE);
  err = vkAllocateCommandBuffers(lime_device.device, &allocate_info, command_buffers);
  ASSERT_VK_RESULT(err, "allocating command buffers");
}

static void
record_command_buffer(VkCommandBuffer command_buffer, int swap_index)
{
  VkCommandBufferBeginInfo begin_info;
  VkClearValue clear_values[2];
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
  clear_values[0].color.float32[0] = 128.0f / 255.0;
  clear_values[0].color.float32[1] = 218.0f / 255.0;
  clear_values[0].color.float32[2] = 251.0f / 255.0;
  clear_values[0].color.float32[3] = 1.0f;
  clear_values[1].depthStencil.depth = 1.0f;
  clear_values[1].depthStencil.stencil = 0;

  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.pNext = NULL;
  render_pass_info.renderPass = lime_pipelines.render_pass;
  render_pass_info.framebuffer = lime_resources.swapchain_framebuffers[swap_index];
  render_pass_info.renderArea.offset.x = 0;
  render_pass_info.renderArea.offset.y = 0;
  render_pass_info.renderArea.extent = lime_resources.swapchain_extent;
  render_pass_info.clearValueCount = sizeof(clear_values) / sizeof(clear_values[0]);
  render_pass_info.pClearValues = clear_values;

  viewport.x = viewport.y = 0.0f;
  viewport.width = lime_resources.swapchain_extent.width;
  viewport.height = lime_resources.swapchain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent = lime_resources.swapchain_extent;

  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lime_pipelines.pipeline);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      lime_pipelines.pipeline_layout, 0, 1,
      &lime_resources.camera_descriptor_sets[swap_index], 0, NULL);
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &lime_vertex_buffers.vertex_buffer,
      &(VkDeviceSize){0});
  vkCmdBindIndexBuffer(command_buffer, lime_vertex_buffers.index_buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  vkCmdDrawIndexed(command_buffer, lime_vertex_buffers.index_count, 1, 0, 0, 0);
  vkCmdEndRenderPass(command_buffer);
  err = vkEndCommandBuffer(command_buffer);
  ASSERT_VK_RESULT(err, "recording command buffer");
}

static void
record_command_buffers(void)
{
  int i;
  for (i = 0; i < lime_resources.swapchain_image_count; i++)
    record_command_buffer(command_buffers[i], i);
}

void
lime_init_renderer(void)
{
  create_graphics_command_pool();
  allocate_command_buffers();
  record_command_buffers();
  create_synchronization_objects();
}

void
lime_draw_frame(struct camera_uniform_data camera)
{
  uint32_t swapchain_index;
  VkSubmitInfo submit_info;
  VkPresentInfoKHR present_info;
  VkResult err;

  vkWaitForFences(lime_device.device, 1, &frame_finished_fence, VK_TRUE, UINT64_MAX);
  vkResetFences(lime_device.device, 1, &frame_finished_fence);
  err = vkAcquireNextImageKHR(lime_device.device, lime_resources.swapchain,
      UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &swapchain_index);
  ASSERT_VK_RESULT(err, "acquiring next swapchain image");
  set_camera_uniform_data(swapchain_index, camera);
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
  present_info.pSwapchains = &lime_resources.swapchain;
  present_info.pImageIndices = &swapchain_index;
  present_info.pResults = NULL;
  err = vkQueuePresentKHR(lime_device.graphics_queue, &present_info);
  ASSERT_VK_RESULT(err, "submitting present request");
}

void
lime_destroy_renderer(void)
{
  vkDestroySemaphore(lime_device.device, image_available_semaphore, NULL);
  vkDestroySemaphore(lime_device.device, render_finished_semaphore, NULL);
  vkDestroyFence(lime_device.device, frame_finished_fence, NULL);
  vkDestroyCommandPool(lime_device.device, graphics_command_pool, NULL);
}
