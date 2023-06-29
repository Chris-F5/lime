#include <vulkan/vulkan.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include <stdlib.h>
#include "lime.h"

static void allocate_queue_family_table_records(
    struct lime_queue_family_table *table, int required);
static void add_device_queue_families(struct lime_queue_family_table *table,
    VkPhysicalDevice physical_device, VkInstance instance);

static void
allocate_queue_family_table_records(struct lime_queue_family_table *table,
   int required)
{
  if (required > table->allocated) {
    table->allocated = required;
    table->physical_device = xrealloc(table->physical_device,
        table->allocated * sizeof(VkPhysicalDevice));
    table->family_index = xrealloc(table->family_index,
        table->allocated * sizeof(int));
    table->properties = xrealloc(table->properties,
        table->allocated * sizeof(VkQueueFamilyProperties));
  }
}

static void
add_device_queue_families(struct lime_queue_family_table *table,
    VkPhysicalDevice physical_device, VkInstance instance)
{
  int family_count, i;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, NULL);
  allocate_queue_family_table_records(table, table->count + family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count,
      table->properties + table->count);
  for (i = 0; i < family_count; i++ ) {
    table->physical_device[table->count + i] = physical_device;
    table->family_index[table->count + i] = i;
  }
  table->count += family_count;
}

void
create_physical_device_table(struct lime_physical_device_table *table,
    VkInstance instance, VkSurfaceKHR surface)
{
  VkResult err;
  int i;
  err = vkEnumeratePhysicalDevices(instance, &table->count, NULL);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating physical devices");
    exit(1);
  }
  table->physical_device = xmalloc(table->count * sizeof(VkPhysicalDevice));
  table->properties = xmalloc(table->count * sizeof(VkPhysicalDeviceProperties));
  err = vkEnumeratePhysicalDevices(instance, &table->count,
      table->physical_device);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "enumerating physical devices");
    exit(1);
  }
  for (i = 0; i < table->count; i++)
    vkGetPhysicalDeviceProperties(table->physical_device[i], &table->properties[i]);
}

void
destroy_physical_device_table(struct lime_physical_device_table *table)
{
  free(table->physical_device);
  free(table->properties);
}

void
create_queue_family_table(struct lime_queue_family_table *queue_family_table,
    const struct lime_physical_device_table *physical_device_table,
    VkInstance instance, VkSurfaceKHR surface)
{
  int i;
  queue_family_table->count = 0;
  queue_family_table->allocated = 0;
  queue_family_table->physical_device = NULL;
  queue_family_table->family_index = NULL;
  queue_family_table->properties = NULL;
  for (i = 0; i < physical_device_table->count; i++)
    add_device_queue_families(queue_family_table,
        physical_device_table->physical_device[i], instance);
}

void
destroy_queue_family_table(struct lime_queue_family_table *table)
{
  free(table->physical_device);
  free(table->family_index);
  free(table->properties);
}

int
check_physical_device_extension_support(VkPhysicalDevice physical_device,
    int required_extension_count, const char * const *required_extension_names)
{
  VkResult err;
  int available_extension_count, r, a;
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
  for (r = 0; r < required_extension_count; r++) {
    for (a = 0; a < available_extension_count; a++)
      if (strcmp(required_extension_names[r],
            available_extensions[a].extensionName) == 0)
        break;
    if (a == available_extension_count)
      return 0;
  }
  return 1;
}

int
select_queue_family_with_flags(const struct lime_queue_family_table *table,
    VkPhysicalDevice physical_device, uint32_t required_flags)
{
  int i;
  for (i = 0; i < table->count; i++)
    if (table->physical_device[i] == physical_device
        && table->properties[i].queueFlags & required_flags == required_flags)
      return table->family_index[i];
  return -1;
}

int
select_queue_family_with_present_support(
    const struct lime_queue_family_table *table,
    VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  int i;
  VkResult err;
  VkBool32 surface_support;
  for (i = 0; i < table->count; i++) {
    if (table->physical_device[i] != physical_device)
      continue;
    err = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface,
        &surface_support);
    if (err != VK_SUCCESS) {
      PRINT_VK_ERROR(err, "getting queue family surface support");
      exit(1);
    }
    if (surface_support)
      return table->family_index[i];
  }
  return -1;
}
