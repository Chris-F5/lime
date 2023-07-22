#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "matrix.h"
#include "lime.h"
#include "utils.h"

static void create_render_pass(void);
static void create_descriptor_set_layouts(void);
static void create_pipeline_layout(void);
static VkShaderModule create_shader_module(const char *file_name);
static void create_pipeline(void);

static VkShaderModule hello_vert_module;
static VkShaderModule hello_frag_module;

struct lime_pipelines lime_pipelines;

static void
create_render_pass(void)
{
  VkAttachmentDescription attachments[2];
  VkAttachmentReference color_attachments[1];
  VkAttachmentReference depth_attachment;
  VkSubpassDependency subpass_dependencies[1];
  VkSubpassDescription subpass;
  VkRenderPassCreateInfo create_info;
  VkResult err;

  attachments[0].flags = 0;
  attachments[0].format = lime_device.surface_format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  attachments[1].flags = 0;
  attachments[1].format = lime_device.depth_format;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  color_attachments[0].attachment = 0;
  color_attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  depth_attachment.attachment = 1;
  depth_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependencies[0].dstSubpass = 0;
  subpass_dependencies[0].srcStageMask 
    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpass_dependencies[0].dstStageMask
    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpass_dependencies[0].srcAccessMask = 0;
  subpass_dependencies[0].dstAccessMask
    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  subpass_dependencies[0].dependencyFlags = 0;

  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = NULL;
  subpass.colorAttachmentCount = sizeof(color_attachments) / sizeof(color_attachments[0]);
  subpass.pColorAttachments = color_attachments;
  subpass.pResolveAttachments = NULL;
  subpass.pDepthStencilAttachment = &depth_attachment;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = NULL;

  create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.attachmentCount = sizeof(attachments) / sizeof(attachments[0]);
  create_info.pAttachments = attachments;
  create_info.subpassCount = 1;
  create_info.pSubpasses = &subpass;
  create_info.dependencyCount = sizeof(subpass_dependencies) / sizeof(subpass_dependencies[0]);
  create_info.pDependencies = subpass_dependencies;
  assert(lime_pipelines.render_pass == VK_NULL_HANDLE);
  err = vkCreateRenderPass(lime_device.device, &create_info, NULL, &lime_pipelines.render_pass);
  ASSERT_VK_RESULT(err, "creating render pass");
}

static void
create_descriptor_set_layouts(void)
{
  VkDescriptorSetLayoutBinding bindings[1];
  VkDescriptorSetLayoutCreateInfo create_info;
  VkResult err;

  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags
    = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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
}

static void
create_pipeline_layout(void)
{
  VkPipelineLayoutCreateInfo create_info;
  VkResult err;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = 1;
  create_info.pSetLayouts = &lime_pipelines.camera_descriptor_set_layout;
  create_info.pushConstantRangeCount = 0;
  create_info.pPushConstantRanges = NULL;
  assert(lime_pipelines.pipeline_layout == VK_NULL_HANDLE);
  err = vkCreatePipelineLayout(lime_device.device, &create_info, NULL,
      &lime_pipelines.pipeline_layout);
  ASSERT_VK_RESULT(err, "creating pipeline layout");
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
create_pipeline(void)
{
  VkPipelineShaderStageCreateInfo shader_stages[2];
  VkVertexInputBindingDescription vertex_input_bindings[2];
  VkVertexInputAttributeDescription vertex_input_attributes[2];
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
  VkResult err;

  shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stages[0].pNext = NULL;
  shader_stages[0].flags = 0;
  shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shader_stages[0].module = hello_vert_module;
  shader_stages[0].pName = "main";
  shader_stages[0].pSpecializationInfo = NULL;
  shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stages[1].pNext = NULL;
  shader_stages[1].flags = 0;
  shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shader_stages[1].module = hello_frag_module;
  shader_stages[1].pName = "main";
  shader_stages[1].pSpecializationInfo = NULL;

  vertex_input_bindings[0].binding = 0;
  vertex_input_bindings[0].stride = 4 * sizeof(float);
  vertex_input_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertex_input_bindings[1].binding = 1;
  vertex_input_bindings[1].stride = 4 * sizeof(float);
  vertex_input_bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertex_input_attributes[0].location = 0;
  vertex_input_attributes[0].binding = 0;
  vertex_input_attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  vertex_input_attributes[0].offset = 0;
  vertex_input_attributes[1].location = 1;
  vertex_input_attributes[1].binding = 1;
  vertex_input_attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  vertex_input_attributes[1].offset = 0;

  vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input.pNext = NULL;
  vertex_input.flags = 0;
  vertex_input.vertexBindingDescriptionCount = sizeof(vertex_input_bindings) / sizeof(vertex_input_bindings[0]);
  vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
  vertex_input.vertexAttributeDescriptionCount = sizeof(vertex_input_attributes) / sizeof(vertex_input_attributes[0]);
  vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;

  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.pNext = NULL;
  input_assembly.flags = 0;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.pNext = NULL;
  viewport_state.flags = 0;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = NULL;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = NULL;

  rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization.pNext = NULL;
  rasterization.flags = 0;
  rasterization.depthClampEnable = VK_FALSE;
  rasterization.rasterizerDiscardEnable = VK_FALSE;
  rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterization.depthBiasEnable = VK_FALSE;
  rasterization.depthBiasConstantFactor = 0.0f;
  rasterization.depthBiasClamp = 0.0f;
  rasterization.depthBiasSlopeFactor = 0.0f;
  rasterization.lineWidth = 1.0f;

  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.pNext = NULL;
  multisample.flags = 0;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample.sampleShadingEnable = VK_FALSE;
  multisample.minSampleShading = 0.0f;
  multisample.pSampleMask = NULL;
  multisample.alphaToCoverageEnable = VK_FALSE;
  multisample.alphaToOneEnable = VK_FALSE;

  depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.pNext = NULL;
  depth_stencil.flags = 0;
  depth_stencil.depthTestEnable = VK_TRUE;
  depth_stencil.depthWriteEnable = VK_TRUE;
  depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.stencilTestEnable = VK_FALSE;
  memset(&depth_stencil.front, 0, sizeof(depth_stencil.front));
  memset(&depth_stencil.back, 0, sizeof(depth_stencil.front));
  depth_stencil.minDepthBounds = 0.0f;
  depth_stencil.maxDepthBounds = 1.0f;

  color_blend_attachments[0].blendEnable = VK_FALSE;
  color_blend_attachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments[0].colorBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
  color_blend_attachments[0].colorWriteMask
    = VK_COLOR_COMPONENT_R_BIT
    | VK_COLOR_COMPONENT_G_BIT
    | VK_COLOR_COMPONENT_B_BIT
    | VK_COLOR_COMPONENT_A_BIT;

  color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend.pNext = NULL;
  color_blend.flags = 0;
  color_blend.logicOpEnable = VK_FALSE;
  color_blend.logicOp = VK_LOGIC_OP_COPY;
  color_blend.attachmentCount = sizeof(color_blend_attachments) / sizeof(color_blend_attachments[0]);
  color_blend.pAttachments = color_blend_attachments;
  color_blend.blendConstants[0] = 0.0f;
  color_blend.blendConstants[1] = 0.0f;
  color_blend.blendConstants[2] = 0.0f;
  color_blend.blendConstants[3] = 0.0f;

  dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
  dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.pNext = NULL;
  dynamic_state.flags = 0;
  dynamic_state.dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]);
  dynamic_state.pDynamicStates = dynamic_states;

  create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.stageCount = sizeof(shader_stages) / sizeof(shader_stages[0]);
  create_info.pStages = shader_stages;
  create_info.pVertexInputState = &vertex_input;
  create_info.pInputAssemblyState = &input_assembly;
  create_info.pTessellationState = NULL;
  create_info.pViewportState = &viewport_state;
  create_info.pRasterizationState = &rasterization;
  create_info.pMultisampleState = &multisample;
  create_info.pDepthStencilState = &depth_stencil;
  create_info.pColorBlendState = &color_blend;
  create_info.pDynamicState = &dynamic_state;
  create_info.layout = lime_pipelines.pipeline_layout;
  create_info.renderPass = lime_pipelines.render_pass;
  create_info.subpass = 0;
  create_info.basePipelineHandle = VK_NULL_HANDLE;
  create_info.basePipelineIndex = 0;

  assert(lime_pipelines.pipeline == VK_NULL_HANDLE);
  err = vkCreateGraphicsPipelines(lime_device.device, VK_NULL_HANDLE, 1,
      &create_info, NULL, &lime_pipelines.pipeline);
  ASSERT_VK_RESULT(err, "creating graphics pipelin");
}

void
lime_init_pipelines(void)
{
  create_render_pass();
  create_descriptor_set_layouts();
  create_pipeline_layout();
  hello_vert_module = create_shader_module("hello.vert.spv");
  hello_frag_module = create_shader_module("hello.frag.spv");
  create_pipeline();
}

void
lime_destroy_pipelines(void)
{
  vkDestroyPipeline(lime_device.device, lime_pipelines.pipeline, NULL);
  vkDestroyShaderModule(lime_device.device, hello_vert_module, NULL);
  vkDestroyShaderModule(lime_device.device, hello_frag_module, NULL);
  vkDestroyPipelineLayout(lime_device.device, lime_pipelines.pipeline_layout, NULL);
  vkDestroyDescriptorSetLayout(lime_device.device,
      lime_pipelines.camera_descriptor_set_layout, NULL);
  vkDestroyRenderPass(lime_device.device, lime_pipelines.render_pass, NULL);
}
