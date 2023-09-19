#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "obj_types.h"
#include "block_allocation.h"
#include "lime.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TEXTURE_IMAGE_FORMAT VK_FORMAT_R8G8B8A8_SRGB

static void create_transfer_command_pool(void);
static void allocate_staging_buffer(VkDeviceSize size);
static void allocate_texture_image(int width, int height, VkImage *image,
    VkDeviceMemory *memory, VkImageView *view);
static void init_texture_image(int width, int height, const stbi_uc *pixels,
    VkImage image);
static void create_texture_sampler(VkSampler *sampler);
static void create_texture_descriptor_pool(void);
static void allocate_texture_descriptor_set(void);
static void write_texture_descriptor_set(VkSampler sampler, VkImageView view);

static VkCommandPool transfer_command_pool;
static VkDeviceSize staging_buffer_size;
static VkBuffer staging_buffer;
static VkDeviceMemory staging_buffer_memory;
static VkImage texture_image;
static VkDeviceMemory texture_image_memory;
static VkImageView texture_image_view;
static VkSampler texture_sampler;
static VkDescriptorPool texture_descriptor_pool;

struct lime_textures lime_textures;

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
  ASSERT_VK_RESULT(err, "creating texture transfer command pool");
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
  ASSERT_VK_RESULT(err, "creating texture staging buffer");

  vkGetBufferMemoryRequirements(lime_device.device, staging_buffer, &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(staging_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL, &staging_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating texture staging buffer memory");
  err = vkBindBufferMemory(lime_device.device, staging_buffer, staging_buffer_memory, 0);
  ASSERT_VK_RESULT(err, "binding texture staging buffer memory");
}

static void
allocate_texture_image(int width, int height, VkImage *image, VkDeviceMemory *memory,
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
  create_info.imageType = VK_IMAGE_TYPE_2D;
  /* TODO: Check format supported. */
  create_info.format = TEXTURE_IMAGE_FORMAT;
  create_info.extent.width = width;
  create_info.extent.height = height;
  create_info.extent.depth = 1;
  create_info.mipLevels = 1;
  create_info.arrayLayers = 1;
  create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  assert(*image == VK_NULL_HANDLE);
  err = vkCreateImage(lime_device.device, &create_info, NULL, image);
  ASSERT_VK_RESULT(err, "creating texture image");

  vkGetImageMemoryRequirements(lime_device.device, *image, &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  assert(*memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL, memory);
  ASSERT_VK_RESULT(err, "allocating texture image memory");
  err = vkBindImageMemory(lime_device.device, *image, *memory, 0);
  ASSERT_VK_RESULT(err, "binding texture image memory");

  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = NULL;
  view_create_info.flags = 0;
  view_create_info.image = *image;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = TEXTURE_IMAGE_FORMAT;
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
  ASSERT_VK_RESULT(err, "creating texture image view");
}

static void
init_texture_image(int width, int height, const stbi_uc *pixels, VkImage image)
{
  void *mapped;
  VkCommandBufferAllocateInfo command_buffer_allocate_info;
  VkCommandBuffer command_buffer;
  VkCommandBufferBeginInfo begin_info;
  VkImageMemoryBarrier barrier;
  VkBufferImageCopy region;
  VkSubmitInfo submit_info;
  VkResult err;

  assert(width * height * 4 <= staging_buffer_size);
  err = vkMapMemory(lime_device.device, staging_buffer_memory, 0, width * height * 4,
      0, &mapped);
  ASSERT_VK_RESULT(err, "mapping texture staging buffer memory");
  memcpy(mapped, pixels, width * height * 4);
  vkUnmapMemory(lime_device.device, staging_buffer_memory);

  command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  command_buffer_allocate_info.pNext = NULL;
  command_buffer_allocate_info.commandPool = transfer_command_pool;
  command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  command_buffer_allocate_info.commandBufferCount = 1;
  err = vkAllocateCommandBuffers(lime_device.device, &command_buffer_allocate_info, &command_buffer);
  ASSERT_VK_RESULT(err, "allocating texture transfer command buffer");

  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = NULL;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  begin_info.pInheritanceInfo = NULL;
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  ASSERT_VK_RESULT(err, "begining texture transfer command buffer");

  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = NULL;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
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

  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset.x = region.imageOffset.y = region.imageOffset.z = 0;
  region.imageExtent.width = width;
  region.imageExtent.height = height;
  region.imageExtent.depth = 1;
  vkCmdCopyBufferToImage(command_buffer, staging_buffer, image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = NULL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(command_buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0, 0, NULL, 0, NULL, 1, &barrier);

  err = vkEndCommandBuffer(command_buffer);
  ASSERT_VK_RESULT(err, "ending texture transfer command buffer");

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
  ASSERT_VK_RESULT(err, "submitting texture transfer command buffer");
  err = vkQueueWaitIdle(lime_device.graphics_queue);
  ASSERT_VK_RESULT(err, "awaiting texture transfer");
  vkFreeCommandBuffers(lime_device.device, transfer_command_pool, 1, &command_buffer);
}

static void
create_texture_sampler(VkSampler *sampler)
{
  VkSamplerCreateInfo create_info;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.magFilter = VK_FILTER_NEAREST;
  create_info.minFilter = VK_FILTER_LINEAR;
  create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  create_info.mipLodBias = 0.0f;
  /* TODO: Enable anisotropy filtering and it's logical device feature. */
  create_info.anisotropyEnable = VK_FALSE;
  create_info.maxAnisotropy = lime_device.properties.limits.maxSamplerAnisotropy;
  create_info.compareEnable = VK_FALSE;
  create_info.compareOp = VK_COMPARE_OP_ALWAYS;
  create_info.minLod = 0.0f;
  create_info.maxLod = 0.0f;
  create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  create_info.unnormalizedCoordinates = VK_FALSE;
  assert(*sampler == VK_NULL_HANDLE);
  err = vkCreateSampler(lime_device.device, &create_info, NULL, sampler);
  ASSERT_VK_RESULT(err, "creating texture sampler");
}

static void
create_texture_descriptor_pool(void)
{
  VkDescriptorPoolSize pool_sizes[1];
  VkDescriptorPoolCreateInfo create_info;
  VkResult err;

  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[0].descriptorCount = 1;
  create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.maxSets = 1;
  create_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
  create_info.pPoolSizes = pool_sizes;
  assert(texture_descriptor_pool == VK_NULL_HANDLE);
  err = vkCreateDescriptorPool(lime_device.device, &create_info, NULL, &texture_descriptor_pool);
  ASSERT_VK_RESULT(err, "creating texture descriptor pool");
}

static void
allocate_texture_descriptor_set(void)
{
  VkDescriptorSetAllocateInfo allocate_info;
  VkResult err;

  allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.descriptorPool = texture_descriptor_pool;
  allocate_info.descriptorSetCount = 1;
  allocate_info.pSetLayouts = &lime_pipelines.texture_descriptor_set_layout;
  assert(lime_textures.texture_descriptor_set == VK_NULL_HANDLE);
  err = vkAllocateDescriptorSets(lime_device.device, &allocate_info,
      &lime_textures.texture_descriptor_set);
  ASSERT_VK_RESULT(err, "allocating texture descriptor set");
}

static void
write_texture_descriptor_set(VkSampler sampler, VkImageView view)
{
  VkDescriptorImageInfo image_info;
  VkWriteDescriptorSet write;
  image_info.sampler = sampler;
  image_info.imageView = view;
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = NULL;
  write.dstSet = lime_textures.texture_descriptor_set;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &image_info;
  write.pBufferInfo = NULL;
  write.pTexelBufferView = NULL;
  vkUpdateDescriptorSets(lime_device.device, 1, &write, 0, NULL);
}

void
lime_init_textures(const char *fname)
{
  int width, height, channels;
  stbi_uc *pixels;

  create_transfer_command_pool();
  allocate_staging_buffer(1024 * 1024 * 4);

  pixels = stbi_load(fname, &width, &height, &channels, STBI_rgb_alpha);
  if (pixels == NULL) {
    fprintf(stderr, "Failed to load texture image file '%s'.\n", fname);
    exit(1);
  }

  allocate_texture_image(width, height, &texture_image, &texture_image_memory, &texture_image_view);
  init_texture_image(width, height, pixels, texture_image);
  create_texture_sampler(&texture_sampler);
  create_texture_descriptor_pool();
  allocate_texture_descriptor_set();
  write_texture_descriptor_set(texture_sampler, texture_image_view);

  stbi_image_free(pixels);
}

void
lime_destroy_textures(void)
{
  vkDestroyDescriptorPool(lime_device.device, texture_descriptor_pool, NULL);
  vkDestroySampler(lime_device.device, texture_sampler, NULL);
  vkDestroyBuffer(lime_device.device, staging_buffer, NULL);
  vkFreeMemory(lime_device.device, staging_buffer_memory, NULL);
  vkDestroyImageView(lime_device.device, texture_image_view, NULL);
  vkDestroyImage(lime_device.device, texture_image, NULL);
  vkFreeMemory(lime_device.device, texture_image_memory, NULL);
  vkDestroyCommandPool(lime_device.device, transfer_command_pool, NULL);
}
