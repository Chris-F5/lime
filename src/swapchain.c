#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include "lime.h"

static void allocate_swapchain_table_records(struct lime_swapchain_table *table,
    int required);
static void allocate_swapchain_image_table_records(
    struct lime_swapchain_image_table *table, int required);
static void add_swapchain_images(struct lime_swapchain_image_table *table,
    VkDevice logical_device, VkSwapchainKHR swapchain, VkFormat format);

static void
allocate_swapchain_table_records(struct lime_swapchain_table *table, int required)
{
  if (required > table->allocated) {
    table->allocated = required;
    table->swapchain = xrealloc(table->swapchain,
        table->allocated * sizeof(VkSwapchainKHR));
    table->logical_device = xrealloc(table->logical_device,
        table->allocated * sizeof(VkDevice));
    table->surface_format = xrealloc(table->surface_format,
        table->allocated * sizeof(VkSurfaceFormatKHR));
    table->extent = xrealloc(table->extent,
        table->allocated * sizeof(VkExtent2D));
  }
}

static void
allocate_swapchain_image_table_records(struct lime_swapchain_image_table *table,
    int required)
{
  if (required > table->allocated) {
    table->allocated = required;
    table->image = xrealloc(table->image, table->allocated * sizeof(VkImage));
    table->image_view = xrealloc(table->image_view,
        table->allocated * sizeof(VkImageView));
    table->swapchain = xrealloc(table->swapchain,
        table->allocated * sizeof(VkSwapchainKHR));
  }
}

static void
add_swapchain_images(struct lime_swapchain_image_table *table,
    VkDevice logical_device, VkSwapchainKHR swapchain, VkFormat format)
{
  uint32_t image_count;
  VkResult err;
  int i;
  VkImageViewCreateInfo view_create_info;
  err = vkGetSwapchainImagesKHR(logical_device, swapchain, &image_count, NULL);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "getting swapchain images");
    exit(1);
  }
  allocate_swapchain_image_table_records(table, table->count + image_count);
  err = vkGetSwapchainImagesKHR(logical_device, swapchain, &image_count,
      table->image + table->count);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "getting swapchain images");
    exit(1);
  }
  view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_create_info.pNext = NULL;
  view_create_info.flags = 0;
  view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_create_info.format = format;
  view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_create_info.subresourceRange.baseMipLevel = 0;
  view_create_info.subresourceRange.levelCount = 1;
  view_create_info.subresourceRange.baseArrayLayer = 0;
  view_create_info.subresourceRange.layerCount = 1;
  for (i = table->count; i < table->count + image_count; i++) {
    table->swapchain[i] = swapchain;
    view_create_info.image = table->image[i];
    err = vkCreateImageView(logical_device, &view_create_info, NULL,
        &table->image_view[i]);
    if (err != VK_SUCCESS) {
      PRINT_VK_ERROR(err, "creating swapchain image view");
      exit(1);
    }
  }
  table->count += image_count;
}

void
create_swapchain_table(struct lime_swapchain_table *table)
{
  table->count = 0;
  table->allocated = 0;
  table->swapchain = NULL;
  table->logical_device = NULL;
  table->surface_format = NULL;
  table->extent = NULL;
  allocate_swapchain_table_records(table, 2);
}

void
destroy_swapchain_table(struct lime_swapchain_table *table)
{
  int i;
  for (i = 0; i < table->count; i++)
    vkDestroySwapchainKHR(table->logical_device[i], table->swapchain[i], NULL);
  free(table->swapchain);
  free(table->logical_device);
  free(table->surface_format);
  free(table->extent);
}

void
create_swapchain_image_table(struct lime_swapchain_image_table *table)
{
  table->count = 0;
  table->allocated = 0;
  table->image = NULL;
  table->image_view = NULL;
  table->swapchain = NULL;
  allocate_swapchain_image_table_records(table, 12);
}

void
destroy_swapchain_image_table(
    struct lime_swapchain_image_table *swapchain_image_table,
    const struct lime_swapchain_table *swapchain_table)
{
  int i, swapchain_table_index;
  VkDevice logical_device;
  for (i = 0; i < swapchain_image_table->count; i++) {
    swapchain_table_index = find_swapchain_table_index(swapchain_table,
        swapchain_image_table->swapchain[i]);
    if (swapchain_table_index < 0) {
      fprintf(stderr, "failed to destroy swapchain image: swapchain not found\n");
      exit(1);
    }
    logical_device = swapchain_table->logical_device[swapchain_table_index];
    vkDestroyImageView(logical_device, swapchain_image_table->image_view[i], NULL);
  }
  free(swapchain_image_table->image);
  free(swapchain_image_table->image_view);
  free(swapchain_image_table->swapchain);
}

int
find_swapchain_table_index(const struct lime_swapchain_table *table,
    VkSwapchainKHR swapchain)
{
  int i;
  for (i = 0; i < table->count; i++)
    if (table->swapchain[i] == swapchain)
      return i;
  return -1;
}

void
create_swapchain(struct lime_swapchain_table *swapchain_table,
    struct lime_swapchain_image_table *image_table,
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    VkDevice logical_device, VkSwapchainCreateFlagsKHR flags,
    VkSurfaceFormatKHR surface_format, VkImageUsageFlags usage_flags,
    VkPresentModeKHR present_mode, int queue_family_count,
    const uint32_t *queue_family_indices, VkBool32 clipped,
    VkCompositeAlphaFlagBitsKHR composite_alpha)
{
  VkResult err;
  VkSurfaceCapabilitiesKHR surface_capabilities;
  VkSwapchainCreateInfoKHR create_info;
  VkSwapchainKHR swapchain;
  int swapchain_table_index;
  err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
      &surface_capabilities);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "getting physical device surface capabilities for swapchain creation");
    exit(1);
  }
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.pNext = NULL;
  create_info.flags = flags;
  create_info.surface = surface;
  if (surface_capabilities.maxImageCount == surface_capabilities.minImageCount)
    create_info.minImageCount = surface_capabilities.minImageCount;
  else
    create_info.minImageCount = surface_capabilities.minImageCount + 1;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  if (surface_capabilities.currentExtent.width == 0xFFFFFFFF) {
    create_info.imageExtent.width = 500;
    create_info.imageExtent.height = 500;
  } else {
    create_info.imageExtent = surface_capabilities.currentExtent;
  }
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = usage_flags;
  if (queue_family_count == 1) {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  } else {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = queue_family_count;
    create_info.pQueueFamilyIndices = queue_family_indices;
  }
  create_info.preTransform = surface_capabilities.currentTransform;
  create_info.compositeAlpha = composite_alpha;
  create_info.presentMode = present_mode;
  create_info.clipped = clipped;
  /*
   * TODO: search for swapchain in table with same surface and use it in
   * oldSwapchain before destroying it.
   */
  create_info.oldSwapchain = VK_NULL_HANDLE;

  allocate_swapchain_table_records(swapchain_table, swapchain_table->count + 1);
  swapchain_table_index = swapchain_table->count++;
  err = vkCreateSwapchainKHR(logical_device, &create_info, NULL, &swapchain);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "creating swapchain");
    exit(1);
  }
  swapchain_table->swapchain[swapchain_table_index] = swapchain;
  swapchain_table->logical_device[swapchain_table_index] = logical_device;
  swapchain_table->surface_format[swapchain_table_index] = surface_format;
  swapchain_table->extent[swapchain_table_index] = create_info.imageExtent;

  add_swapchain_images(image_table, logical_device, swapchain, surface_format.format);
}
