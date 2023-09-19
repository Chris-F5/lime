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

static VkDeviceMemory vertex_buffer_memory;
static VkDeviceMemory index_buffer_memory;

static void allocate_vertex_buffers(long vertex_count, long index_count);

struct lime_vertex_buffers lime_vertex_buffers;

static void
allocate_vertex_buffers(long vertex_memory, long index_memory)
{
  VkBufferCreateInfo create_info;
  VkMemoryRequirements memory_requirements;
  VkMemoryAllocateInfo allocate_info;
  VkResult err;

  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = vertex_memory;
  create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(lime_vertex_buffers.vertex_buffer == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &lime_vertex_buffers.vertex_buffer);
  ASSERT_VK_RESULT(err, "creating vertex buffer");

  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.size = index_memory;
  create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices = NULL;
  assert(lime_vertex_buffers.index_buffer == VK_NULL_HANDLE);
  err = vkCreateBuffer(lime_device.device, &create_info, NULL, &lime_vertex_buffers.index_buffer);
  ASSERT_VK_RESULT(err, "creating vertex index buffer");

  vkGetBufferMemoryRequirements(lime_device.device, lime_vertex_buffers.vertex_buffer,
      &memory_requirements);
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.pNext = NULL;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = lime_device_find_memory_type(
      memory_requirements.memoryTypeBits,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  assert(vertex_buffer_memory == VK_NULL_HANDLE);
  err = vkAllocateMemory(lime_device.device, &allocate_info, NULL,
      &vertex_buffer_memory);
  ASSERT_VK_RESULT(err, "allocating vertex buffer memory");
  err = vkBindBufferMemory(lime_device.device, lime_vertex_buffers.vertex_buffer,
      vertex_buffer_memory, 0);
  ASSERT_VK_RESULT(err, "binding vertex buffer memory");

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
  ASSERT_VK_RESULT(err, "binding vertex index buffer memory");
}

void
lime_init_vertex_buffers(long vertex_count, long index_count)
{
  allocate_vertex_buffers(vertex_count * sizeof(struct vertex), index_count * sizeof(uint32_t));
  init_block_allocation_table(&lime_vertex_buffers.vertex_table, vertex_count);
  init_block_allocation_table(&lime_vertex_buffers.index_table, index_count);
}

void
lime_create_graphics_vertex_obj(struct graphics_vertex_obj *gvo,
  const struct indexed_vertex_obj *ivo)
{
  void *mapped;
  VkResult err;

  gvo->vertex_count = ivo->vertex_count;
  gvo->index_count = ivo->index_count;
  gvo->vertex_offset = allocate_block(&lime_vertex_buffers.vertex_table, ivo->vertex_count);
  gvo->index_offset = allocate_block(&lime_vertex_buffers.index_table, ivo->index_count);
  if (gvo->vertex_offset < 0) {
    fprintf(stderr, "Vertex buffer overflow.");
    exit(1);
  }
  if (gvo->index_offset < 0) {
    fprintf(stderr, "Index buffer overflow.");
    exit(1);
  }

  err = vkMapMemory(lime_device.device, vertex_buffer_memory,
      gvo->vertex_offset * sizeof(struct vertex),
      ivo->vertex_count * sizeof(struct vertex), 0, &mapped);
  ASSERT_VK_RESULT(err, "mapping vertex buffer");
  memcpy(mapped, ivo->vertices, ivo->vertex_count * sizeof(struct vertex));
  vkUnmapMemory(lime_device.device, vertex_buffer_memory);
  err = vkMapMemory(lime_device.device, index_buffer_memory,
      gvo->index_offset * sizeof(uint32_t),
      ivo->index_count * sizeof(uint32_t), 0, &mapped);
  ASSERT_VK_RESULT(err, "mapping vertex index buffer");
  memcpy(mapped, ivo->indices, ivo->index_count * sizeof(uint32_t));
  vkUnmapMemory(lime_device.device, index_buffer_memory);
}

void
lime_free_graphics_vertex_obj(struct graphics_vertex_obj *gvo)
{
  free_block(&lime_vertex_buffers.vertex_table, gvo->vertex_offset);
  free_block(&lime_vertex_buffers.index_table, gvo->index_offset);
}

void
lime_destroy_vertex_buffers(void)
{
  vkDestroyBuffer(lime_device.device, lime_vertex_buffers.vertex_buffer, NULL);
  vkDestroyBuffer(lime_device.device, lime_vertex_buffers.index_buffer, NULL);
  vkFreeMemory(lime_device.device, vertex_buffer_memory, NULL);
  vkFreeMemory(lime_device.device, index_buffer_memory, NULL);
  destroy_block_allocation_table(&lime_vertex_buffers.vertex_table);
  destroy_block_allocation_table(&lime_vertex_buffers.index_table);
}
