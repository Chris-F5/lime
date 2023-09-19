#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "obj_types.h"
#include "block_allocation.h"
#include "lime.h"
#include <string.h>
#include <assert.h>

#define VOXEL_BLOCK_IMAGE_FORMAT VK_FORMAT_R8_UINT

static void allocate_voxel_block_uniform_buffer(void);
static void create_transfer_command_pool(void);
static void allocate_staging_buffer(VkDeviceSize size);
static void allocate_voxel_image(int size, VkImage *image, VkDeviceMemory *memory,
    VkImageView *view);
static void init_voxel_image(VkImage image);
static void fill_voxel_block_image(int size, const char *voxels, VkImage image);
static void create_voxel_block_descriptor_pool(void);
static void allocate_voxel_block_descriptor_set(void);
static void write_voxel_block_descriptor_set(void);

static VkBuffer voxel_block_uniform_buffer;
static VkDeviceMemory voxel_block_uniform_buffer_memory;
static VkCommandPool transfer_command_pool;
static VkDeviceSize staging_buffer_size;
static VkBuffer staging_buffer;
static VkDeviceMemory staging_buffer_memory;
static VkImage voxel_image;
static VkDeviceMemory voxel_image_memory;
static VkImageView voxel_image_view;
static VkDescriptorPool voxel_block_descriptor_pool;

struct lime_voxel_blocks lime_voxel_blocks;

static void
allocate_voxel_block_uniform_buffer(void)
{
  VkBufferCreateInfo create_info;
  VkMemoryRequirements memory_requirements;
  VkMemoryAllocateInfo allocate_info;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = sizeof(struct voxel_block_uniform_data);
  create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(voxel_block_uniform_buffer == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &voxel_block_uniform_buffer);
  ASSERT_VK_RESULT(err, "creating voxel block uniform buffer");

  vkGetBufferMemoryRequirements(lime_device.device, voxel_block_uniform_buffer,
      &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(voxel_block_uniform_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL,
      &voxel_block_uniform_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating voxel block uniform buffer memory");
  err = vkBindBufferMemory(lime_device.device, voxel_block_uniform_buffer,
      voxel_block_uniform_buffer_memory, 0);
  ASSERT_VK_RESULT(err, "binding voxel block uniform buffer memory");
}

static void
create_transfer_command_pool(void)
{
  VkCommandPoolCreateInfo create_info;
  VkResult err;
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  create_info.queueFamilyIndex = lime_device.graphics_family_index;
  assert(transfer_command_pool == VK_NULL_HANDLE);
  err = vkCreateCommandPool(lime_device.device, &create_info, NULL, &transfer_command_pool);
  ASSERT_VK_RESULT(err, "creating voxel block transfer command pool");
}

static void
allocate_staging_buffer(VkDeviceSize size)
{
  VkBufferCreateInfo create_info;
  VkMemoryRequirements memory_requirements;
  VkMemoryAllocateInfo allocate_info;
  VkResult err;

  staging_buffer_size = size;

  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = size;
  create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(staging_buffer == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &staging_buffer);
  ASSERT_VK_RESULT(err, "creating voxel block staging buffer");

  vkGetBufferMemoryRequirements(lime_device.device, staging_buffer, &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(staging_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL, &staging_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating voxel block staging buffer memory");
  err = vkBindBufferMemory(lime_device.device, staging_buffer, staging_buffer_memory, 0);
  ASSERT_VK_RESULT(err, "binding voxel block staging buffer memory");
}

static void
allocate_voxel_image(int size, VkImage *image, VkDeviceMemory *memory,
    VkImageView *view)
{
  VkImageCreateInfo create_info;
  VkMemoryRequirements memory_requirements;
  VkMemoryAllocateInfo allocate_info;
  VkImageViewCreateInfo view_create_info;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.imageType = VK_IMAGE_TYPE_3D;
  /* TODO: Check format supported. */
  create_info.format = VOXEL_BLOCK_IMAGE_FORMAT;
  create_info.extent.width = size;
  create_info.extent.height = size;
  create_info.extent.depth = size;
  create_info.mipLevels = 1;
  create_info.arrayLayers = 1;
  create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  assert(*image == VK_NULL_HANDLE);
  err = vkCreateImage(lime_device.device, &create_info, NULL, image);
  ASSERT_VK_RESULT(err, "creating voxel block image");

  vkGetImageMemoryRequirements(lime_device.device, *image, &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  assert(*memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL, memory);
  ASSERT_VK_RESULT(err, "allocating voxel block image memory");
  err = vkBindImageMemory(lime_device.device, *image, *memory, 0);
  ASSERT_VK_RESULT(err, "binding voxel block image memory");

  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = NULL;
  view_create_info.flags = 0;
  view_create_info.image = *image;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
  view_create_info.format = VOXEL_BLOCK_IMAGE_FORMAT;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  assert(*view == VK_NULL_HANDLE);
  err = vkCreateImageView(lime_device.device, &view_create_info, NULL, view);
  ASSERT_VK_RESULT(err, "creating voxel block image view");
}

static void
init_voxel_image(VkImage image)
{
  VkCommandBufferAllocateInfo command_buffer_allocate_info;
  VkCommandBuffer command_buffer;
  VkCommandBufferBeginInfo begin_info;
  VkImageMemoryBarrier barrier;
  VkSubmitInfo submit_info;
  VkResult err;

  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.pNext = NULL;
  command_buffer_allocate_info.commandPool = transfer_command_pool;
  command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_allocate_info.commandBufferCount = 1;
  err = vkAllocateCommandBuffers(lime_device.device, &command_buffer_allocate_info, &command_buffer);
  ASSERT_VK_RESULT(err, "allocating voxel image init command buffer");

  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = NULL;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  begin_info.pInheritanceInfo = NULL;
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  ASSERT_VK_RESULT(err, "begining voxel image init command buffer");

  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = NULL;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(command_buffer,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
      0, 0, NULL, 0, NULL, 1, &barrier);

  err = vkEndCommandBuffer(command_buffer);
  ASSERT_VK_RESULT(err, "ending voxel image init command buffer");

  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = NULL;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = NULL;
  submit_info.pWaitDstStageMask = NULL;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = NULL;
  err = vkQueueSubmit(lime_device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  ASSERT_VK_RESULT(err, "submitting voxel image init command buffer");
  err = vkQueueWaitIdle(lime_device.graphics_queue);
  ASSERT_VK_RESULT(err, "awaiting voxel image init");
  vkFreeCommandBuffers(lime_device.device, transfer_command_pool, 1, &command_buffer);
}

static void
fill_voxel_block_image(int size, const char *voxels, VkImage image)
{
  char *mapped;
  VkCommandBufferAllocateInfo command_buffer_allocate_info;
  VkCommandBuffer command_buffer;
  VkCommandBufferBeginInfo begin_info;
  VkBufferImageCopy region;
  VkSubmitInfo submit_info;
  VkResult err;

  assert(size * size * size <= staging_buffer_size);
  err = vkMapMemory(lime_device.device, staging_buffer_memory, 0, size * size * size,
      0, (void **)&mapped);
  ASSERT_VK_RESULT(err, "mapping voxel block staging buffer memory");
  memcpy(mapped, voxels, size * size * size);
  vkUnmapMemory(lime_device.device, staging_buffer_memory);

  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.pNext = NULL;
  command_buffer_allocate_info.commandPool = transfer_command_pool;
  command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_allocate_info.commandBufferCount = 1;
  err = vkAllocateCommandBuffers(lime_device.device, &command_buffer_allocate_info, &command_buffer);
  ASSERT_VK_RESULT(err, "allocating voxel image transfer command buffer");

  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = NULL;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  begin_info.pInheritanceInfo = NULL;
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  ASSERT_VK_RESULT(err, "begining voxel image transfer command buffer");

  /* TODO: Proper synchronization before and after. */

  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset.x = region.imageOffset.y = region.imageOffset.z = 0;
  region.imageExtent.width = size;
  region.imageExtent.height = size;
  region.imageExtent.depth = size;
  vkCmdCopyBufferToImage(command_buffer, staging_buffer, image,
      VK_IMAGE_LAYOUT_GENERAL, 1, &region);

  err = vkEndCommandBuffer(command_buffer);
  ASSERT_VK_RESULT(err, "ending voxel image transfer command buffer");

  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = NULL;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = NULL;
  submit_info.pWaitDstStageMask = NULL;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = NULL;
  err = vkQueueSubmit(lime_device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  ASSERT_VK_RESULT(err, "submitting voxel image transfer command buffer");
  err = vkQueueWaitIdle(lime_device.graphics_queue);
  ASSERT_VK_RESULT(err, "awaiting voxel image transfer");
  vkFreeCommandBuffers(lime_device.device, transfer_command_pool, 1, &command_buffer);
}

static void
create_voxel_block_descriptor_pool(void)
{
  VkDescriptorPoolSize pool_sizes[2];
  VkDescriptorPoolCreateInfo create_info;
  VkResult err;

  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = 1;
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  pool_sizes[1].descriptorCount = 1;
  create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.maxSets = 1;
  create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
  create_info.pPoolSizes = pool_sizes;
  assert(voxel_block_descriptor_pool == VK_NULL_HANDLE);
  err = vkCreateDescriptorPool(lime_device.device, &create_info, NULL,
      &voxel_block_descriptor_pool);
  ASSERT_VK_RESULT(err, "creating voxel block descriptor pool");
}

static void
allocate_voxel_block_descriptor_set(void)
{
  VkDescriptorSetAllocateInfo allocate_info;
  VkResult err;

  allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.descriptorPool = voxel_block_descriptor_pool;
  allocate_info.descriptorSetCount = 1;
  allocate_info.pSetLayouts = &lime_pipelines.voxel_block_descriptor_set_layout;
  assert(lime_voxel_blocks.descriptor_set == VK_NULL_HANDLE);
  err = vkAllocateDescriptorSets(lime_device.device, &allocate_info,
      &lime_voxel_blocks.descriptor_set);
  ASSERT_VK_RESULT(err, "allocating voxel block descriptor set");
}

static void
write_voxel_block_descriptor_set(void)
{
  VkDescriptorBufferInfo buffer_info;
  VkDescriptorImageInfo image_info;
  VkWriteDescriptorSet writes[2];
  buffer_info.buffer = voxel_block_uniform_buffer;
  buffer_info.offset = 0;
  buffer_info.range = sizeof(struct voxel_block_uniform_data);
  writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[0].pNext = NULL;
  writes[0].dstSet = lime_voxel_blocks.descriptor_set;
  writes[0].dstBinding = 0;
  writes[0].dstArrayElement = 0;
  writes[0].descriptorCount = 1;
  writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writes[0].pImageInfo = NULL;
  writes[0].pBufferInfo = &buffer_info;
  writes[0].pTexelBufferView = NULL;
  image_info.sampler = VK_NULL_HANDLE;
  image_info.imageView = voxel_image_view;
  image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writes[1].pNext = NULL;
  writes[1].dstSet = lime_voxel_blocks.descriptor_set;
  writes[1].dstBinding = 1;
  writes[1].dstArrayElement = 0;
  writes[1].descriptorCount = 1;
  writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  writes[1].pImageInfo = &image_info;
  writes[1].pBufferInfo = NULL;
  writes[1].pTexelBufferView = NULL;
  vkUpdateDescriptorSets(lime_device.device, sizeof(writes) / sizeof(writes[0]),
      writes, 0, NULL);
}

void
lime_init_voxel_blocks(struct voxel_block_uniform_data uniform_data, int size, const char *voxels)
{
  struct voxel_block_uniform_data *mapped;
  VkResult err;

  allocate_voxel_block_uniform_buffer();
  create_transfer_command_pool();
  assert(size <= 256);
  allocate_staging_buffer(256 * 256 * 256);
  allocate_voxel_image(size, &voxel_image, &voxel_image_memory,
    &voxel_image_view);
  init_voxel_image(voxel_image);
  fill_voxel_block_image(size, voxels, voxel_image);
  create_voxel_block_descriptor_pool();
  allocate_voxel_block_descriptor_set();
  write_voxel_block_descriptor_set();

  err = vkMapMemory(lime_device.device, voxel_block_uniform_buffer_memory,
      0, sizeof(struct voxel_block_uniform_data),
      0, (void **)&mapped);
  ASSERT_VK_RESULT(err, "mapping voxel block uniform buffer data");
  *mapped = uniform_data;
  vkUnmapMemory(lime_device.device, voxel_block_uniform_buffer_memory);
}

void
lime_destroy_voxel_blocks(void)
{
  vkDestroyDescriptorPool(lime_device.device, voxel_block_descriptor_pool, NULL);
  vkDestroyImageView(lime_device.device, voxel_image_view, NULL);
  vkDestroyImage(lime_device.device, voxel_image, NULL);
  vkFreeMemory(lime_device.device, voxel_image_memory, NULL);
  vkDestroyBuffer(lime_device.device, staging_buffer, NULL);
  vkFreeMemory(lime_device.device, staging_buffer_memory, NULL);
  vkDestroyBuffer(lime_device.device, voxel_block_uniform_buffer, NULL);
  vkFreeMemory(lime_device.device, voxel_block_uniform_buffer_memory, NULL);
  vkDestroyCommandPool(lime_device.device, transfer_command_pool, NULL);
}
