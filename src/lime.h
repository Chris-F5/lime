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

struct lime_device {
  VkSurfaceKHR surface;
  uint32_t graphics_family_index;
  VkDevice device;
  VkQueue graphics_queue;
  VkFormat surface_format;
  VkPresentModeKHR present_mode;
};

struct lime_pipelines {
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
};

extern struct lime_device lime_device;
extern struct lime_pipelines lime_pipelines;

/* device.c */
void lime_init_device(GLFWwindow *window);
VkSurfaceCapabilitiesKHR lime_get_current_surface_capabilities(void);
void lime_destroy_device(void);

/* pipelines.c */
void lime_init_pipelines(void);
void lime_destroy_pipelines(void);

/* renderer.c */
void lime_init_renderer(void);
void lime_draw_frame(void);
void lime_destroy_renderer(void);

/* lime.c */
char *vkresult_to_string(VkResult result);
