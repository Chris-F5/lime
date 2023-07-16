#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "lime.h"

static void create_swapchain(VkSurfaceCapabilitiesKHR surface_capabilities);
static void create_framebuffers(void);

static VkImage swapchain_images[MAX_SWAPCHAIN_IMAGES];
static VkImageView swapchain_image_views[MAX_SWAPCHAIN_IMAGES];

struct lime_resources lime_resources;

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

  lime_resources.swapchain_extent = create_info.imageExtent;
  err = vkCreateSwapchainKHR(lime_device.device, &create_info, NULL, &lime_resources.swapchain);
  ASSERT_VK_RESULT(err, "creating swapchain");

  err = vkGetSwapchainImagesKHR(lime_device.device, lime_resources.swapchain,
      &lime_resources.swapchain_image_count, NULL);
  ASSERT_VK_RESULT(err, "getting swapchain images");
  if (lime_resources.swapchain_image_count >= MAX_SWAPCHAIN_IMAGES) {
    fprintf(stderr, "Too many swapchain images.\n");
    exit(1);
  }
  err = vkGetSwapchainImagesKHR(lime_device.device, lime_resources.swapchain,
      &lime_resources.swapchain_image_count, swapchain_images);
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
  for (i = 0; i < lime_resources.swapchain_image_count; i++) {
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
  create_info.width = lime_resources.swapchain_extent.width;
  create_info.height = lime_resources.swapchain_extent.height;
  create_info.layers = 1;
  for (i = 0; i < lime_resources.swapchain_image_count; i++) {
    create_info.pAttachments = &swapchain_image_views[i];
    assert(lime_resources.swapchain_framebuffers[i] == VK_NULL_HANDLE);
    err = vkCreateFramebuffer(lime_device.device, &create_info, NULL,
        &lime_resources.swapchain_framebuffers[i]);
    ASSERT_VK_RESULT(err, "creating swapcahin framebuffer");
  }
}

void
lime_init_resources(void)
{
  VkSurfaceCapabilitiesKHR surface_capabilities;
  surface_capabilities = lime_get_current_surface_capabilities();
  create_swapchain(surface_capabilities);
  create_framebuffers();
}

void
lime_destroy_resources(void)
{
  int i;
  for (i = 0; i < lime_resources.swapchain_image_count; i++) {
    vkDestroyFramebuffer(lime_device.device, lime_resources.swapchain_framebuffers[i], NULL);
    vkDestroyImageView(lime_device.device, swapchain_image_views[i], NULL);
  }
  vkDestroySwapchainKHR(lime_device.device, lime_resources.swapchain, NULL);
}
