#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include "lime.h"

static void allocate_logical_device_table_records(
    struct lime_logical_device_table *table, int required);
static void allocate_queue_table_records(struct lime_queue_table *table,
    int required);

static void
allocate_logical_device_table_records(struct lime_logical_device_table *table,
    int required)
{
  if (required > table->allocated) {
    table->allocated = required;
    table->logical_device = xrealloc(table->logical_device,
        table->allocated * sizeof(VkDevice));
    table->physical_device = xrealloc(table->physical_device,
        table->allocated * sizeof(VkPhysicalDevice));
  }
}

static void
allocate_queue_table_records(struct lime_queue_table *table, int required)
{
  if (required > table->allocated) {
    table->allocated = required;
    table->logical_device = xrealloc(table->logical_device,
        table->allocated * sizeof(VkDevice));
    table->family_index = xrealloc(table->family_index,
        table->allocated * sizeof(int));
    table->queue_index = xrealloc(table->queue_index,
        table->allocated * sizeof(int));
    table->queue = xrealloc(table->queue, table->allocated * sizeof(VkQueue));
  }
}

void
create_logical_device_table(struct lime_logical_device_table *table)
{
  table->count = 0;
  table->allocated = 0;
  table->logical_device = NULL;
  table->physical_device = NULL;
  allocate_logical_device_table_records(table, 2);
}

void
destroy_logical_device_table(struct lime_logical_device_table *table)
{
  int i;
  for (i = 0; i < table->count; i++)
    vkDestroyDevice(table->logical_device[i], NULL);
  free(table->logical_device);
  free(table->physical_device);
}

void
create_queue_table(struct lime_queue_table *table)
{
  table->count = 0;
  table->allocated = 0;
  table->logical_device = NULL;
  table->family_index = NULL;
  table->queue_index = NULL;
  table->queue = NULL;
  allocate_queue_table_records(table, 16);
}

void
destroy_queue_table(struct lime_queue_table *table)
{
  free(table->logical_device);
  free(table->family_index);
  free(table->queue_index);
  free(table->queue);
}

void
create_logical_device(struct lime_logical_device_table *logical_device_table,
    struct lime_queue_table *queue_table, VkPhysicalDevice physical_device,
    int queue_family_count, const int *queue_families, int extension_count,
    const char * const* extension_names, VkPhysicalDeviceFeatures features)
{
  VkDeviceCreateInfo create_info;
  VkDeviceQueueCreateInfo *queue_create_infos;
  float queue_priority;
  int i, logical_device_index, queue_table_index;
  VkResult err;
  VkDevice logical_device;

  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.queueCreateInfoCount = queue_family_count;
  queue_create_infos = xmalloc(queue_family_count * sizeof(VkDeviceQueueCreateInfo));
  queue_priority = 1.0f;
  for (i = 0; i < queue_family_count; i++) {
    queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[i].pNext = NULL;
    queue_create_infos[i].flags = 0;
    queue_create_infos[i].queueFamilyIndex = queue_families[i];
    queue_create_infos[i].queueCount = 1;
    queue_create_infos[i].pQueuePriorities = &queue_priority;
  }
  create_info.pQueueCreateInfos = queue_create_infos;
  /* TODO: Enable device layers (depricated) for compatibility. */
  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = NULL;
  create_info.enabledExtensionCount = extension_count;
  create_info.ppEnabledExtensionNames = extension_names;

  err = vkCreateDevice(physical_device, &create_info, NULL, &logical_device);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "creating logical device");
    exit(1);
  }
  free(queue_create_infos);

  allocate_logical_device_table_records(logical_device_table,
      logical_device_table->count + 1);
  logical_device_index = logical_device_table->count++;
  logical_device_table->logical_device[logical_device_index] = logical_device;
  logical_device_table->physical_device[logical_device_index] = physical_device;

  allocate_queue_table_records(queue_table, queue_table->count + queue_family_count);
  for (i = 0; i < queue_family_count; i++) {
    queue_table_index = queue_table->count++;
    queue_table->logical_device[queue_table_index] = logical_device;
    queue_table->family_index[queue_table_index] = queue_families[i];
    queue_table->queue_index[queue_table_index] = 0;
    vkGetDeviceQueue(logical_device, queue_families[i], 0,
        &queue_table->queue[queue_table_index]);
  }
}
