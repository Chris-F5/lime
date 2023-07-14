/*
 * The following must be included before this file:
 * #include <stdio.h>
 * #include <vulkan/vulkan.h>
 */

#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }
#define ASSERT_VK_RESULT(err, string) if (err != VK_SUCCESS) { \
  PRINT_VK_ERROR(err, string); \
  exit(1); \
}

extern struct {
  VkDevice device;
} vk_globals;

/* video.c */
void init_video(void);

/* lime.c */
char *vkresult_to_string(VkResult result);
