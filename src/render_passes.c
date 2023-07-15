#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

void
init_render_passes(void)
{
  VkAttachmentDescription attachments[1];
  VkAttachmentReference color_attachments[1];
  VkSubpassDescription subpass;
  VkRenderPassCreateInfo create_info;
  VkResult err;

  attachments[0].flags = 0;
  attachments[0].format = vk_globals.surface_format.format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  color_attachments[0].attachment = 0;
  color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = NULL;
  subpass.colorAttachmentCount = sizeof(color_attachments) / sizeof(color_attachments[0]);
  subpass.pColorAttachments = color_attachments;
  subpass.pResolveAttachments = NULL;
  subpass.pDepthStencilAttachment = NULL;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = NULL;

  create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
  create_info.pAttachments = attachments;
  create_info.subpassCount = 1;
  create_info.pSubpasses = &subpass;
  create_info.dependencyCount = 0;
  create_info.pDependencies = NULL;
  err = vkCreateRenderPass(vk_globals.device, &create_info, NULL, &vk_globals.render_pass);
  ASSERT_VK_RESULT(err, "creating render pass");
}

void
destroy_render_passes(void)
{
  vkDestroyRenderPass(vk_globals.device, vk_globals.render_pass, NULL);
}
