#include <vulkan/vulkan.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "lime.h"

static void allocate_queue_family_table_records(
    struct lime_queue_family_table *table, int required);
static void add_device_queue_families(struct lime_queue_family_table *table,
    int hardware_id, VkPhysicalDevice physical_device, VkInstance instance,
    VkSurfaceKHR surface);

static void
allocate_queue_family_table_records(struct lime_queue_family_table *table,
    int required)
{
  if (required > table->allocated) {
    table->allocated = required;
    table->hardware_id = xrealloc(table->hardware_id,
        table->allocated * sizeof(int));
    table->family_index = xrealloc(table->family_index,
        table->allocated * sizeof(int));
    table->properties = xrealloc(table->properties,
        table->allocated * sizeof(VkQueueFamilyProperties));
  }
}

static void
add_device_queue_families(struct lime_queue_family_table *table, int hardware_id,
    VkPhysicalDevice physical_device, VkInstance instance, VkSurfaceKHR surface)
{
  int family_count, i;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, NULL);
  allocate_queue_family_table_records(table, table->count + family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count,
      table->properties + table->count);
  for (i = 0; i < family_count; i++ ) {
    table->hardware_id[table->count + i] = hardware_id;
    table->family_index[table->count + i] = i;
  }
  table->count += family_count;
}

void
create_hardware_table(struct lime_hardware_table *table, VkInstance instance,
    VkSurfaceKHR surface)
{
  VkResult err;
  int i;
  err = vkEnumeratePhysicalDevices(instance, &table->count, NULL);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating physical devices");
    exit(1);
  }
  table->device = xmalloc(table->count * sizeof(VkPhysicalDevice));
  table->surface_capabilities = xmalloc(table->count *
      sizeof(VkSurfaceCapabilitiesKHR));
  err = vkEnumeratePhysicalDevices(instance, &table->count, table->device);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating physical devices");
    exit(1);
  }
  for (i = 0; i < table->count; i++) {
    err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(table->device[i], surface,
        &table->surface_capabilities[i]);
    if (err != VK_SUCCESS) {
      PRINT_VK_ERROR(err, "getting hardware surface capabilities");
      exit(1);
    }
  }
}

void
destroy_hardware_table(struct lime_hardware_table *table)
{
  free(table->device);
  free(table->surface_capabilities);
}

void
create_queue_family_table(struct lime_queue_family_table *queue_family_table,
    const struct lime_hardware_table *hardware_table, VkInstance instance,
    VkSurfaceKHR surface)
{
  int hardware_id;
  queue_family_table->count = 0;
  queue_family_table->allocated = 0;
  queue_family_table->hardware_id = NULL;
  queue_family_table->family_index = NULL;
  queue_family_table->properties = NULL;
  allocate_queue_family_table_records(queue_family_table, 1);
  for (hardware_id = 0; hardware_id < hardware_table->count; hardware_id++)
    add_device_queue_families(queue_family_table, hardware_id,
        hardware_table->device[hardware_id], instance, surface);
}

void
destroy_queue_family_table(struct lime_queue_family_table *table)
{
  free(table->hardware_id);
  free(table->family_index);
  free(table->properties);
}
