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

struct vk_globals {
  uint32_t graphics_family_index;
  VkSurfaceFormatKHR surface_format;
  VkDevice device;
  VkQueue graphics_queue;
  VkRenderPass render_pass;
};

extern struct vk_globals vk_globals;

/* video.c */
void init_video(GLFWwindow *window);
void destroy_video(void);

/* render_passes.c */
void init_render_passes(void);
void destroy_render_passes(void);

/* lime.c */
char *vkresult_to_string(VkResult result);
