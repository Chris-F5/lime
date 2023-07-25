#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "obj_types.h"
#include "lime.h"
#include "utils.h"

struct pipeline_create_info {
  VkPipelineShaderStageCreateInfo shader_stages[2];
  VkVertexInputBindingDescription vertex_input_bindings[1];
  VkVertexInputAttributeDescription vertex_input_attributes[3];
  VkPipelineVertexInputStateCreateInfo vertex_input;
  VkPipelineInputAssemblyStateCreateInfo input_assembly;
  VkPipelineViewportStateCreateInfo viewport_state;
  VkPipelineRasterizationStateCreateInfo rasterization;
  VkPipelineMultisampleStateCreateInfo multisample;
  VkPipelineDepthStencilStateCreateInfo depth_stencil;
  VkPipelineColorBlendAttachmentState color_blend_attachments[1];
  VkPipelineColorBlendStateCreateInfo color_blend;
  VkDynamicState dynamic_states[2];
  VkPipelineDynamicStateCreateInfo dynamic_state;
  VkGraphicsPipelineCreateInfo create_info;
};

struct render_pass_create_info {
  VkAttachmentDescription attachments[2];
  VkSubpassDependency subpass_dependencies[1];
  VkSubpassDescription subpass;
  VkRenderPassCreateInfo create_info;
};

static void init_default_render_pass_create_info(struct render_pass_create_info *info);
static void create_render_passes(void);
static void create_descriptor_set_layouts(void);
static void create_pipeline_layouts(void);
static VkShaderModule create_shader_module(const char *file_name);
static void init_default_pipeline_create_info(struct pipeline_create_info *info);
static void create_pipelines(void);

static VkShaderModule hello_vert_module;
static VkShaderModule hello_frag_module;
static VkShaderModule voxel_block_vert_module;
static VkShaderModule voxel_block_frag_module;

struct lime_pipelines lime_pipelines;

static void
init_default_render_pass_create_info(struct render_pass_create_info *info)
{
  memset(info->attachments, 0, sizeof(info->attachments));
  memset(info->subpass_dependencies, 0, sizeof(info->subpass_dependencies));

  info->subpass.flags = 0;
  info->subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  info->subpass.inputAttachmentCount = 0;
  info->subpass.pInputAttachments = NULL;
  info->subpass.colorAttachmentCount = 0;
  info->subpass.pColorAttachments = NULL;
  info->subpass.pResolveAttachments = NULL;
  info->subpass.pDepthStencilAttachment = NULL;
  info->subpass.preserveAttachmentCount = 0;
  info->subpass.pPreserveAttachments = NULL;

  info->create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info->create_info.pNext = NULL;
  info->create_info.flags = 0;
  info->create_info.attachmentCount = 0;
  info->create_info.pAttachments = info->attachments;
  info->create_info.subpassCount = 1;
  info->create_info.pSubpasses = &info->subpass;
  info->create_info.dependencyCount = 0;
  info->create_info.pDependencies = info->subpass_dependencies;
}

static void
create_render_passes(void)
{
  struct render_pass_create_info info;
  VkAttachmentReference color_attachments[1];
  VkAttachmentReference depth_attachment;
  VkResult err;

  init_default_render_pass_create_info(&info);
  info.attachments[0].format = lime_device.surface_format.format;
  info.attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  info.attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  info.attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  info.attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  info.attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  info.attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  info.attachments[1].format = lime_device.depth_format;
  info.attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  info.attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  info.attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  info.attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  info.attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  info.attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  info.attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  color_attachments[0].attachment = 0;
  color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  depth_attachment.attachment = 1;
  depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  info.subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  info.subpass_dependencies[0].dstSubpass = 0;
  info.subpass_dependencies[0].srcStageMask 
    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  info.subpass_dependencies[0].dstStageMask
    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
    | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  info.subpass_dependencies[0].srcAccessMask = 0;
  info.subpass_dependencies[0].dstAccessMask
    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  info.subpass.colorAttachmentCount = 1;
  info.subpass.pColorAttachments = color_attachments;
  info.subpass.pDepthStencilAttachment = &depth_attachment;
  info.create_info.attachmentCount = 2;
  info.create_info.dependencyCount = 1;
  assert(lime_pipelines.render_pass == VK_NULL_HANDLE);
  err = vkCreateRenderPass(lime_device.device, &info.create_info, NULL, &lime_pipelines.render_pass);
  ASSERT_VK_RESULT(err, "creating render pass");

  init_default_render_pass_create_info(&info);
  info.attachments[0].format = lime_device.surface_format.format;
  info.attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  info.attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  info.attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  info.attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  info.attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  info.attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  info.attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  info.attachments[1].format = lime_device.depth_format;
  info.attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  info.attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  info.attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  info.attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  info.attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  info.attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  info.attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  color_attachments[0].attachment = 0;
  color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  depth_attachment.attachment = 1;
  depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  info.subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  info.subpass_dependencies[0].dstSubpass = 0;
  info.subpass_dependencies[0].srcStageMask 
    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  info.subpass_dependencies[0].dstStageMask
    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
    | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  info.subpass_dependencies[0].srcAccessMask = 0;
  info.subpass_dependencies[0].dstAccessMask
    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  info.subpass.colorAttachmentCount = 1;
  info.subpass.pColorAttachments = color_attachments;
  info.subpass.pDepthStencilAttachment = &depth_attachment;
  info.create_info.attachmentCount = 2;
  info.create_info.dependencyCount = 1;
  assert(lime_pipelines.voxel_block_render_pass == VK_NULL_HANDLE);
  err = vkCreateRenderPass(lime_device.device, &info.create_info, NULL,
      &lime_pipelines.voxel_block_render_pass);
  ASSERT_VK_RESULT(err, "creating voxel block render pass");
}

static void
create_descriptor_set_layouts(void)
{
  VkDescriptorSetLayoutBinding bindings[2];
  VkDescriptorSetLayoutCreateInfo create_info;
  VkResult err;

  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = NULL;
  create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.bindingCount = 1;
  create_info.pBindings = bindings;
  assert(lime_pipelines.camera_descriptor_set_layout == VK_NULL_HANDLE);
  err = vkCreateDescriptorSetLayout(lime_device.device, &create_info, NULL,
      &lime_pipelines.camera_descriptor_set_layout);
  ASSERT_VK_RESULT(err, "creating camera descriptor set layout");

  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = NULL;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.bindingCount = 1;
  create_info.pBindings = bindings;
  assert(lime_pipelines.texture_descriptor_set_layout == VK_NULL_HANDLE);
  err = vkCreateDescriptorSetLayout(lime_device.device, &create_info, NULL,
      &lime_pipelines.texture_descriptor_set_layout);
  ASSERT_VK_RESULT(err, "creating texture descriptor set layout");

  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[0].pImmutableSamplers = NULL;
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  bindings[1].pImmutableSamplers = NULL;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.bindingCount = 2;
  create_info.pBindings = bindings;
  assert(lime_pipelines.voxel_block_descriptor_set_layout == VK_NULL_HANDLE);
  err = vkCreateDescriptorSetLayout(lime_device.device, &create_info, NULL,
      &lime_pipelines.voxel_block_descriptor_set_layout);
  ASSERT_VK_RESULT(err, "creating voxel block descriptor set layout");
}

static void
create_pipeline_layouts(void)
{
  VkDescriptorSetLayout set_layouts[2];
  VkPipelineLayoutCreateInfo create_info;
  VkResult err;

  set_layouts[0] = lime_pipelines.camera_descriptor_set_layout;
  set_layouts[1] = lime_pipelines.texture_descriptor_set_layout;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = 2;
  create_info.pSetLayouts = set_layouts;
  create_info.pushConstantRangeCount = 0;
  create_info.pPushConstantRanges = NULL;
  assert(lime_pipelines.pipeline_layout == VK_NULL_HANDLE);
  err = vkCreatePipelineLayout(lime_device.device, &create_info, NULL,
      &lime_pipelines.pipeline_layout);
  ASSERT_VK_RESULT(err, "creating pipeline layout");

  set_layouts[0] = lime_pipelines.camera_descriptor_set_layout;
  set_layouts[1] = lime_pipelines.voxel_block_descriptor_set_layout;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = 2;
  create_info.pSetLayouts = set_layouts;
  create_info.pushConstantRangeCount = 0;
  create_info.pPushConstantRanges = NULL;
  assert(lime_pipelines.voxel_block_pipeline_layout == VK_NULL_HANDLE);
  err = vkCreatePipelineLayout(lime_device.device, &create_info, NULL,
      &lime_pipelines.voxel_block_pipeline_layout);
  ASSERT_VK_RESULT(err, "creating voxel block pipeline layout");
}

static VkShaderModule
create_shader_module(const char *file_name)
{
  FILE *file;
  long spv_length;
  char *spv;
  VkShaderModuleCreateInfo create_info;
  VkResult err;
  VkShaderModule module;

  /* TODO: Embed spv files in binary. */
  file = fopen(file_name, "r");
  if (file == NULL) {
    fprintf(stderr, "failed to open shader file '%s'\n", file_name);
    exit(1);
  }
  fseek(file, 0, SEEK_END);
  spv_length = ftell(file);
  fseek(file, 0, SEEK_SET);
  spv = xmalloc(spv_length);
  fread(spv, 1, spv_length, file);
  fclose(file);

  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.codeSize = spv_length;
  create_info.pCode = (uint32_t *)spv;
  err = vkCreateShaderModule(lime_device.device, &create_info, NULL, &module);
  ASSERT_VK_RESULT(err, "creating shader module");
  free(spv);
  return module;
}

static void
init_default_pipeline_create_info(struct pipeline_create_info *info)
{
  int i;
  for (i = 0; i < sizeof(info->shader_stages) / sizeof(info->shader_stages[0]); i++) {
    info->shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info->shader_stages[i].pNext = NULL;
    info->shader_stages[i].flags = 0;
    info->shader_stages[i].stage = 0;
    info->shader_stages[i].module = VK_NULL_HANDLE;
    info->shader_stages[i].pName = "main";
    info->shader_stages[i].pSpecializationInfo = NULL;
  }
  memset(info->vertex_input_bindings, 0, sizeof(info->vertex_input_bindings));
  memset(info->vertex_input_attributes, 0, sizeof(info->vertex_input_attributes));

  info->vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info->vertex_input.pNext = NULL;
  info->vertex_input.flags = 0;
  info->vertex_input.vertexBindingDescriptionCount = 0;
  info->vertex_input.pVertexBindingDescriptions = info->vertex_input_bindings;
  info->vertex_input.vertexAttributeDescriptionCount = 0;
  info->vertex_input.pVertexAttributeDescriptions = info->vertex_input_attributes;

  info->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info->input_assembly.pNext = NULL;
  info->input_assembly.flags = 0;
  info->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  info->input_assembly.primitiveRestartEnable = VK_FALSE;

  info->viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  info->viewport_state.pNext = NULL;
  info->viewport_state.flags = 0;
  info->viewport_state.viewportCount = 1;
  info->viewport_state.pViewports = NULL;
  info->viewport_state.scissorCount = 1;
  info->viewport_state.pScissors = NULL;

  info->rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info->rasterization.pNext = NULL;
  info->rasterization.flags = 0;
  info->rasterization.depthClampEnable = VK_FALSE;
  info->rasterization.rasterizerDiscardEnable = VK_FALSE;
  info->rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  info->rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
  info->rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  info->rasterization.depthBiasEnable = VK_FALSE;
  info->rasterization.depthBiasConstantFactor = 0.0f;
  info->rasterization.depthBiasClamp = 0.0f;
  info->rasterization.depthBiasSlopeFactor = 0.0f;
  info->rasterization.lineWidth = 1.0f;

  info->multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info->multisample.pNext = NULL;
  info->multisample.flags = 0;
  info->multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  info->multisample.sampleShadingEnable = VK_FALSE;
  info->multisample.minSampleShading = 0.0f;
  info->multisample.pSampleMask = NULL;
  info->multisample.alphaToCoverageEnable = VK_FALSE;
  info->multisample.alphaToOneEnable = VK_FALSE;

  info->depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info->depth_stencil.pNext = NULL;
  info->depth_stencil.flags = 0;
  info->depth_stencil.depthTestEnable = VK_TRUE;
  info->depth_stencil.depthWriteEnable = VK_TRUE;
  info->depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
  info->depth_stencil.depthBoundsTestEnable = VK_FALSE;
  info->depth_stencil.stencilTestEnable = VK_FALSE;
  memset(&info->depth_stencil.front, 0, sizeof(info->depth_stencil.front));
  memset(&info->depth_stencil.back, 0, sizeof(info->depth_stencil.front));
  info->depth_stencil.minDepthBounds = 0.0f;
  info->depth_stencil.maxDepthBounds = 1.0f;

  info->color_blend_attachments[0].blendEnable = VK_FALSE;
  info->color_blend_attachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  info->color_blend_attachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  info->color_blend_attachments[0].colorBlendOp = VK_BLEND_OP_ADD;
  info->color_blend_attachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  info->color_blend_attachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  info->color_blend_attachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
  info->color_blend_attachments[0].colorWriteMask
    = VK_COLOR_COMPONENT_R_BIT
    | VK_COLOR_COMPONENT_G_BIT
    | VK_COLOR_COMPONENT_B_BIT
    | VK_COLOR_COMPONENT_A_BIT;

  info->color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  info->color_blend.pNext = NULL;
  info->color_blend.flags = 0;
  info->color_blend.logicOpEnable = VK_FALSE;
  info->color_blend.logicOp = VK_LOGIC_OP_COPY;
  info->color_blend.attachmentCount = 1;
  info->color_blend.pAttachments = info->color_blend_attachments;
  info->color_blend.blendConstants[0] = 0.0f;
  info->color_blend.blendConstants[1] = 0.0f;
  info->color_blend.blendConstants[2] = 0.0f;
  info->color_blend.blendConstants[3] = 0.0f;

  info->dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
  info->dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
  info->dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  info->dynamic_state.pNext = NULL;
  info->dynamic_state.flags = 0;
  info->dynamic_state.dynamicStateCount = 2;
  info->dynamic_state.pDynamicStates = info->dynamic_states;

  info->create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  info->create_info.pNext = NULL;
  info->create_info.flags = 0;
  info->create_info.stageCount = 0;
  info->create_info.pStages = info->shader_stages;
  info->create_info.pVertexInputState = &info->vertex_input;
  info->create_info.pInputAssemblyState = &info->input_assembly;
  info->create_info.pTessellationState = NULL;
  info->create_info.pViewportState = &info->viewport_state;
  info->create_info.pRasterizationState = &info->rasterization;
  info->create_info.pMultisampleState = &info->multisample;
  info->create_info.pDepthStencilState = &info->depth_stencil;
  info->create_info.pColorBlendState = &info->color_blend;
  info->create_info.pDynamicState = &info->dynamic_state;
  info->create_info.layout = VK_NULL_HANDLE;
  info->create_info.renderPass = VK_NULL_HANDLE;
  info->create_info.subpass = 0;
  info->create_info.basePipelineHandle = VK_NULL_HANDLE;
  info->create_info.basePipelineIndex = 0;
}

static void
create_pipelines(void)
{
  VkResult err;
  struct pipeline_create_info info;

  init_default_pipeline_create_info(&info);
  info.shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  info.shader_stages[0].module = hello_vert_module;
  info.shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  info.shader_stages[1].module = hello_frag_module;
  info.vertex_input_bindings[0].binding = 0;
  info.vertex_input_bindings[0].stride = sizeof(struct vertex);
  info.vertex_input_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  info.vertex_input_attributes[0].location = 0;
  info.vertex_input_attributes[0].binding = 0;
  info.vertex_input_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  info.vertex_input_attributes[0].offset = offsetof(struct vertex, pos);
  info.vertex_input_attributes[1].location = 1;
  info.vertex_input_attributes[1].binding = 0;
  info.vertex_input_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
  info.vertex_input_attributes[1].offset = offsetof(struct vertex, uv);
  info.vertex_input_attributes[2].location = 2;
  info.vertex_input_attributes[2].binding = 0;
  info.vertex_input_attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  info.vertex_input_attributes[2].offset = offsetof(struct vertex, normal);
  info.vertex_input.vertexBindingDescriptionCount = 1;
  info.vertex_input.vertexAttributeDescriptionCount = 3;
  info.create_info.stageCount = 2;
  info.create_info.layout = lime_pipelines.pipeline_layout;
  info.create_info.renderPass = lime_pipelines.render_pass;
  assert(lime_pipelines.pipeline == VK_NULL_HANDLE);
  err = vkCreateGraphicsPipelines(lime_device.device, VK_NULL_HANDLE, 1,
      &info.create_info, NULL, &lime_pipelines.pipeline);
  ASSERT_VK_RESULT(err, "creating graphics pipeline");

  init_default_pipeline_create_info(&info);
  info.shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  info.shader_stages[0].module = voxel_block_vert_module;
  info.shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  info.shader_stages[1].module = voxel_block_frag_module;
  info.create_info.stageCount = 2;
  info.create_info.layout = lime_pipelines.voxel_block_pipeline_layout;
  info.create_info.renderPass = lime_pipelines.voxel_block_render_pass;
  assert(lime_pipelines.voxel_block_pipeline == VK_NULL_HANDLE);
  err = vkCreateGraphicsPipelines(lime_device.device, VK_NULL_HANDLE, 1,
      &info.create_info, NULL, &lime_pipelines.voxel_block_pipeline);
  ASSERT_VK_RESULT(err, "creating voxel block pipeline");
}

void
lime_init_pipelines(void)
{
  create_render_passes();
  create_descriptor_set_layouts();
  create_pipeline_layouts();
  hello_vert_module = create_shader_module("hello.vert.spv");
  hello_frag_module = create_shader_module("hello.frag.spv");
  voxel_block_vert_module = create_shader_module("voxel_block.vert.spv");
  voxel_block_frag_module = create_shader_module("voxel_block.frag.spv");
  create_pipelines();
}

void
lime_destroy_pipelines(void)
{
  vkDestroyPipeline(lime_device.device, lime_pipelines.pipeline, NULL);
  vkDestroyPipeline(lime_device.device, lime_pipelines.voxel_block_pipeline, NULL);
  vkDestroyShaderModule(lime_device.device, hello_vert_module, NULL);
  vkDestroyShaderModule(lime_device.device, hello_frag_module, NULL);
  vkDestroyShaderModule(lime_device.device, voxel_block_vert_module, NULL);
  vkDestroyShaderModule(lime_device.device, voxel_block_frag_module, NULL);
  vkDestroyPipelineLayout(lime_device.device, lime_pipelines.pipeline_layout, NULL);
  vkDestroyPipelineLayout(lime_device.device, lime_pipelines.voxel_block_pipeline_layout, NULL);
  vkDestroyDescriptorSetLayout(lime_device.device,
      lime_pipelines.camera_descriptor_set_layout, NULL);
  vkDestroyDescriptorSetLayout(lime_device.device,
      lime_pipelines.texture_descriptor_set_layout, NULL);
  vkDestroyDescriptorSetLayout(lime_device.device,
      lime_pipelines.voxel_block_descriptor_set_layout, NULL);
  vkDestroyRenderPass(lime_device.device, lime_pipelines.render_pass, NULL);
  vkDestroyRenderPass(lime_device.device, lime_pipelines.voxel_block_render_pass, NULL);
}
