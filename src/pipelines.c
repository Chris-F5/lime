#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"
#include "utils.h"

static VkShaderModule create_shader_module(const char *file_name);

static VkShaderModule hello_vert_module;
static VkShaderModule hello_frag_module;

void
init_pipeline_layouts(void)
{
  VkPipelineLayoutCreateInfo create_info;
  VkResult err;
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = 0;
  create_info.pSetLayouts = NULL;
  create_info.pushConstantRangeCount = 0;
  create_info.pPushConstantRanges = NULL;
  assert(vk_globals.pipeline_layout == VK_NULL_HANDLE);
  err = vkCreatePipelineLayout(vk_globals.device, &create_info, NULL, &vk_globals.pipeline_layout);
  ASSERT_VK_RESULT(err, "creating pipeline layout");
}

void
destroy_pipeline_layouts(void)
{
  vkDestroyPipelineLayout(vk_globals.device, vk_globals.pipeline_layout, NULL);
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
  err = vkCreateShaderModule(vk_globals.device, &create_info, NULL, &module);
  ASSERT_VK_RESULT(err, "creating shader module");
  free(spv);
  return module;
}

void
init_shader_modules(void)
{
  hello_vert_module = create_shader_module("hello.vert.spv");
  hello_frag_module = create_shader_module("hello.frag.spv");
}

void
destroy_shader_modules(void)
{
  vkDestroyShaderModule(vk_globals.device, hello_vert_module, NULL);
  vkDestroyShaderModule(vk_globals.device, hello_frag_module, NULL);
}

void
create_pipelines(void)
{
  VkPipelineShaderStageCreateInfo shader_stages[2];
  VkPipelineVertexInputStateCreateInfo vertex_input;
  VkPipelineInputAssemblyStateCreateInfo input_assembly;
  VkViewport viewport;
  VkRect2D scissor;
  VkPipelineViewportStateCreateInfo viewport_state;
  VkPipelineRasterizationStateCreateInfo rasterization;
  VkPipelineMultisampleStateCreateInfo multisample;
  VkPipelineColorBlendAttachmentState color_blend_attachments[1];
  VkPipelineColorBlendStateCreateInfo color_blend;
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

  vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input.pNext = NULL;
  vertex_input.flags = 0;
  vertex_input.vertexBindingDescriptionCount = 0;
  vertex_input.pVertexBindingDescriptions = NULL;
  vertex_input.vertexAttributeDescriptionCount = 0;
  vertex_input.pVertexAttributeDescriptions = NULL;

  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.pNext = NULL;
  input_assembly.flags = 0;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  /* TODO: Make viewport size dynamic state, see vulkan tutorial. */
  viewport.x = viewport.y = 0.0f;
  viewport.width = vk_globals.swapchain_extent.width;
  viewport.height = vk_globals.swapchain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent = vk_globals.swapchain_extent;

  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.pNext = NULL;
  viewport_state.flags = 0;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;

  rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization.pNext = NULL;
  rasterization.flags = 0;
  rasterization.depthClampEnable = VK_FALSE;
  rasterization.rasterizerDiscardEnable = VK_FALSE;
  rasterization.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
  create_info.pDepthStencilState = NULL;
  create_info.pColorBlendState = &color_blend;
  create_info.pDynamicState = NULL;
  create_info.layout = vk_globals.pipeline_layout;
  create_info.renderPass = vk_globals.render_pass;
  create_info.subpass = 0;
  create_info.basePipelineHandle = VK_NULL_HANDLE;
  create_info.basePipelineIndex = 0;

  assert(vk_globals.pipeline == VK_NULL_HANDLE);
  err = vkCreateGraphicsPipelines(vk_globals.device, VK_NULL_HANDLE, 1,
      &create_info, NULL, &vk_globals.pipeline);
  ASSERT_VK_RESULT(err, "creating graphics pipelin");
}

void
destroy_pipelines(void)
{
  vkDestroyPipeline(vk_globals.device, vk_globals.pipeline, NULL);
}
