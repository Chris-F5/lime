#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include "lime.h"

static void allocate_shader_module_table_records(
    struct lime_shader_module_table *table, int required);

static void
allocate_shader_module_table_records(struct lime_shader_module_table *table,
    int required)
{
  if (required > table->allocated) {
    table->allocated = required;
    table->shader_module = xrealloc(table->shader_module,
        table->allocated * sizeof(VkShaderModule));
    table->logical_device = xrealloc(table->logical_device,
        table->allocated * sizeof(VkDevice));
    table->file_name = xrealloc(table->file_name,
        table->allocated * sizeof(char *));
  }
}

void
create_shader_module_table(struct lime_shader_module_table *table)
{
  table->count = table->allocated = 0;
  table->shader_module = NULL;
  table->logical_device = NULL;
  table->file_name = NULL;
  allocate_shader_module_table_records(table, 8);
}

void
destroy_shader_module_table(struct lime_shader_module_table *table)
{
  int i;
  for (i = 0; i < table->count; i++)
    vkDestroyShaderModule(table->logical_device[i], table->shader_module[i], NULL);
  free(table->shader_module);
  free(table->logical_device);
  free(table->file_name);
}

VkShaderModule
create_shader_module(struct lime_shader_module_table *table,
    VkDevice logical_device, const char *file_name)
{
  FILE *file;
  long spv_length;
  char *spv;
  VkShaderModuleCreateInfo create_info;
  int table_index;
  VkShaderModule shader_module;
  VkResult err;

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

  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.codeSize = spv_length;
  create_info.pCode = (uint32_t *)spv;
  err = vkCreateShaderModule(logical_device, &create_info, NULL,
      &shader_module);
  if (err != VK_SUCCESS) {
    PRINT_VK_ERROR(err, "creating shader module");
    exit(1);
  }

  allocate_shader_module_table_records(table, table->count + 1);
  table_index = table->count++;
  table->shader_module[table_index] = shader_module;
  table->logical_device[table_index] = logical_device;
  table->file_name[table_index] = file_name;

  free(spv);
  return shader_module;
}
