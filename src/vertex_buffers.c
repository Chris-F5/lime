#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "lime.h"

static VkDeviceMemory vertex_pos_buffer_memory, vertex_color_buffer_memory;
static VkDeviceMemory index_buffer_memory;

static void allocate_vertex_buffers(int vertex_count, int face_count);
static void fill_vertex_buffers(int vertex_count, int face_count, const float *vertex_positions,
    const float *vertex_colors, const uint32_t *vertex_indices);

struct lime_vertex_buffers lime_vertex_buffers;

static void
allocate_vertex_buffers(int vertex_count, int face_count)
{
  VkBufferCreateInfo create_info;
  VkMemoryRequirements memory_requirements;
  VkMemoryAllocateInfo allocate_info;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = vertex_count * 4 * sizeof(float);
  create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(lime_vertex_buffers.vertex_buffers[0] == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &lime_vertex_buffers.vertex_buffers[0]);
  ASSERT_VK_RESULT(err, "creating vertex position buffer");
  lime_vertex_buffers.vertex_buffer_offsets[0] = 0;

  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = vertex_count * 4 * sizeof(float);
  create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(lime_vertex_buffers.vertex_buffers[1] == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &lime_vertex_buffers.vertex_buffers[1]);
  ASSERT_VK_RESULT(err, "creating vertex color buffer");
  lime_vertex_buffers.vertex_buffer_offsets[1] = 0;

  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = face_count * 3 * sizeof(uint32_t);
  create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(lime_vertex_buffers.index_buffer == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &lime_vertex_buffers.index_buffer);
  ASSERT_VK_RESULT(err, "creating vertex index buffer");

  vkGetBufferMemoryRequirements(lime_device.device, lime_vertex_buffers.vertex_buffers[0],
      &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(vertex_pos_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL,
      &vertex_pos_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating vertex position buffer memory");
  err = vkBindBufferMemory(lime_device.device, lime_vertex_buffers.vertex_buffers[0],
      vertex_pos_buffer_memory, 0);

  vkGetBufferMemoryRequirements(lime_device.device, lime_vertex_buffers.vertex_buffers[1],
      &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(vertex_color_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL,
      &vertex_color_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating vertex color buffer memory");
  err = vkBindBufferMemory(lime_device.device, lime_vertex_buffers.vertex_buffers[1],
      vertex_color_buffer_memory, 0);

  vkGetBufferMemoryRequirements(lime_device.device, lime_vertex_buffers.index_buffer,
      &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(index_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL, &index_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating vertex index buffer memory");
  err = vkBindBufferMemory(lime_device.device, lime_vertex_buffers.index_buffer,
      index_buffer_memory, 0);
}

static void
fill_vertex_buffers(int vertex_count, int face_count, const float *vertex_positions,
    const float *vertex_colors, const uint32_t *vertex_indices)
{
  void *mapped;
  VkResult err;
  err = vkMapMemory(lime_device.device, vertex_pos_buffer_memory,
      0, vertex_count * 4 * sizeof(float), 0, &mapped);
  ASSERT_VK_RESULT(err, "mapping vertex position buffer");
  memcpy(mapped, vertex_positions, vertex_count * 4 * sizeof(float));
  vkUnmapMemory(lime_device.device, vertex_pos_buffer_memory);
  err = vkMapMemory(lime_device.device, vertex_color_buffer_memory,
      0, vertex_count * 4 * sizeof(float), 0, &mapped);
  ASSERT_VK_RESULT(err, "mapping vertex color buffer");
  memcpy(mapped, vertex_colors, vertex_count * 4 * sizeof(float));
  vkUnmapMemory(lime_device.device, vertex_color_buffer_memory);
  err = vkMapMemory(lime_device.device, index_buffer_memory,
      0, face_count * 3 * sizeof(uint32_t), 0, &mapped);
  ASSERT_VK_RESULT(err, "mapping vertex index buffer");
  memcpy(mapped, vertex_indices, face_count * 3 * sizeof(uint32_t));
  vkUnmapMemory(lime_device.device, index_buffer_memory);
}

void
lime_init_vertex_buffers(int vertex_count, int face_count, const float *vertex_positions,
    const float *vertex_colors, const uint32_t *vertex_indices)
{
  allocate_vertex_buffers(vertex_count, face_count);
  fill_vertex_buffers(vertex_count, face_count, vertex_positions, vertex_colors, vertex_indices);
  lime_vertex_buffers.vertex_count = vertex_count;
  lime_vertex_buffers.face_count = face_count;
}

void
lime_destroy_vertex_buffers(void)
{
  int i;
  for (i = 0; i < sizeof(lime_vertex_buffers.vertex_buffers) / sizeof(lime_vertex_buffers.vertex_buffers[0]); i++)
    vkDestroyBuffer(lime_device.device, lime_vertex_buffers.vertex_buffers[i], NULL);
  vkDestroyBuffer(lime_device.device, lime_vertex_buffers.index_buffer, NULL);
  vkFreeMemory(lime_device.device, vertex_pos_buffer_memory, NULL);
  vkFreeMemory(lime_device.device, vertex_color_buffer_memory, NULL);
  vkFreeMemory(lime_device.device, index_buffer_memory, NULL);
}
