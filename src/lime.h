/*
 * The following must be included before this file:
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <vulkan/vulkan.h>
 * #include <GLFW/glfw3.h>
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
  int _;
};

struct lime_device {
  VkSurfaceKHR surface;
  VkPhysicalDeviceProperties properties;
  uint32_t graphics_family_index;
  VkDevice device;
  VkQueue graphics_queue;
  VkFormat surface_format;
  VkPresentModeKHR present_mode;
};

struct lime_pipelines {
  VkRenderPass render_pass;
  VkDescriptorSetLayout camera_descriptor_set_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
};

struct lime_resources {
  VkSwapchainKHR swapchain;
  uint32_t swapchain_image_count;
  VkExtent2D swapchain_extent;
  VkFramebuffer swapchain_framebuffers[MAX_SWAPCHAIN_IMAGES];
  VkDescriptorSet camera_descriptor_sets[MAX_SWAPCHAIN_IMAGES];
};

extern struct lime_device lime_device;
extern struct lime_pipelines lime_pipelines;
extern struct lime_resources lime_resources;

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
void lime_destroy_resources(void);

/* renderer.c */
void lime_init_renderer(void);
void lime_draw_frame(void);
void lime_destroy_renderer(void);

/* lime_utils.c */
char *vkresult_to_string(VkResult result);
