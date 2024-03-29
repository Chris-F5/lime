/*
 * The following must be included before this file:
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <stdint.h>
 * #include <vulkan/vulkan.h>
 * #include <GLFW/glfw3.h>
 * #include "matrix.h"
 * #include "obj_types.h"
 * #include "block_allocation.h"
 */

#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }
#define ASSERT_VK_RESULT(err, string) if (err != VK_SUCCESS) { \
  PRINT_VK_ERROR(err, string); \
  exit(1); \
}

#define MAX_SWAPCHAIN_IMAGES 8

struct camera_uniform_data {
  mat4 model;
  mat4 view;
  mat4 proj;
  int color;
};

struct voxel_block_uniform_data {
  mat4 model;
  int scale;
};

struct lime_device {
  VkSurfaceKHR surface;
  VkPhysicalDeviceProperties properties;
  uint32_t graphics_family_index;
  VkDevice device;
  VkQueue graphics_queue;
  VkSurfaceFormatKHR surface_format;
  VkFormat depth_format;
  VkPresentModeKHR present_mode;
};

struct lime_pipelines {
  VkRenderPass render_pass, voxel_block_render_pass;
  VkDescriptorSetLayout camera_descriptor_set_layout, texture_descriptor_set_layout;
  VkDescriptorSetLayout voxel_block_descriptor_set_layout;
  VkPipelineLayout pipeline_layout, voxel_block_pipeline_layout;
  VkPipeline pipeline, voxel_block_pipeline;
};

struct lime_resources {
  VkSwapchainKHR swapchain;
  uint32_t swapchain_image_count;
  VkExtent2D swapchain_extent;
  VkFramebuffer swapchain_framebuffers[MAX_SWAPCHAIN_IMAGES];
  VkFramebuffer voxel_block_framebuffers[MAX_SWAPCHAIN_IMAGES];
  VkDescriptorSet camera_descriptor_sets[MAX_SWAPCHAIN_IMAGES];
};

struct lime_vertex_buffers {
  struct block_allocation_table vertex_table;
  struct block_allocation_table index_table;
  VkBuffer vertex_buffer;
  VkBuffer index_buffer;
};

struct lime_textures {
  VkDescriptorSet texture_descriptor_set;
};

struct lime_voxel_blocks {
  VkDescriptorSet descriptor_set;
};

extern struct lime_device lime_device;
extern struct lime_pipelines lime_pipelines;
extern struct lime_resources lime_resources;
extern struct lime_vertex_buffers lime_vertex_buffers;
extern struct lime_textures lime_textures;
extern struct lime_voxel_blocks lime_voxel_blocks;

/* device.c */
void lime_init_device(GLFWwindow *window);
VkSurfaceCapabilitiesKHR lime_get_current_surface_capabilities(void);
uint32_t lime_device_find_memory_type(uint32_t memory_type_bits,
    VkMemoryPropertyFlags properties);
void lime_destroy_device(void);

/* pipelines.c */
void lime_init_pipelines(void);
void lime_destroy_pipelines(void);

/* resources.c */
void lime_init_resources(void);
void set_camera_uniform_data(int swap_index, struct camera_uniform_data data);
void lime_destroy_resources(void);

/* vertex_buffers.c */
void lime_init_vertex_buffers(long vertex_memory, long index_memory);
void lime_create_graphics_vertex_obj(struct graphics_vertex_obj *gvo,
    const struct indexed_vertex_obj *ivo);
void lime_free_graphics_vertex_obj(struct graphics_vertex_obj *gvo);
void lime_destroy_vertex_buffers(void);

/* textures.c */
void lime_init_textures(const char *fname);
void lime_destroy_textures(void);

/* voxel_blocks.c */
void lime_init_voxel_blocks(struct voxel_block_uniform_data uniform_data, int size,
    const char *voxels);
void lime_destroy_voxel_blocks(void);

/* renderer.c */
void lime_init_renderer(const struct graphics_vertex_obj *gvo);
void lime_draw_frame(struct camera_uniform_data camera);
void lime_destroy_renderer(void);

/* lime_utils.c */
char *vkresult_to_string(VkResult result);
