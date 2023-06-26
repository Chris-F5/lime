#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

/*
struct lime_render_queue_table {
  int count, allocated;
  VkQueue *queue;
};

struct lime_command_pool_table {
  int count, allocated;
  VkCommandPool *command_pool;
};

struct lime_device_table {
  int count, allocated;
  VkDevice *logical_device;
};

struct lime_swapchain_table {
  int count, allocated;
  int *device_id;
  VkSwapchainKHR *swapchain;
  VkExtent2D *extent;
  VkFormat *format;
  uint32_t *image_count;
  VkImage **images;
  VkImageView **image_views;
};

struct lime_render_pass_table {
  int count, allocated;
  int *device_id;
  VkRenderPass *render_pass;
};

struct lime_render_pipeline_table {
  int count, allocated;
  int *device_id;
  VkPipelineLayout *layout;
  VkPipeline *pipeline;
};

struct lime_shader_module_table {
  int count, allocated;
  const char *fname;
  int *device_id;
  VkShaderModule *shader_module;
};

struct lime_command_buffer_table {
  int count, allocated;
  int *device_id;
  VkCommandBuffer *command_buffer;
};
*/

struct lime_queue_family_table {
  int count, allocated;
  VkPhysicalDevice *physical_device;
  int *family_index;
  VkQueueFamilyProperties *properties;
};

struct lime_physical_device_table {
  int count;
  VkPhysicalDevice *physical_device;
  VkPhysicalDeviceProperties *properties;
  VkSurfaceCapabilitiesKHR *surface_capabilities;
};

struct lime_renderer {
  int validation_layers_enabled;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkSurfaceKHR surface;

  VkSurfaceFormatKHR surface_format;
  VkPresentModeKHR present_mode;
  VkFormat depth_image_format;

  struct lime_physical_device_table physical_devices;
  struct lime_queue_family_table queue_families;
};

/* utils.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
char *vkresult_to_string(VkResult result);

/* renderer.c */
void create_renderer(struct lime_renderer *renderer, GLFWwindow* window);
void destroy_renderer(struct lime_renderer *renderer);

/* hardware_table.c */
void create_physical_device_table(struct lime_physical_device_table *table,
    VkInstance instance, VkSurfaceKHR surface);
void destroy_physical_device_table(struct lime_physical_device_table *table);
void create_queue_family_table(struct lime_queue_family_table *queue_family_table,
    const struct lime_physical_device_table *physical_device_table,
    VkInstance instance, VkSurfaceKHR surface);
int select_queue_family_with_flags(const struct lime_queue_family_table *table,
    VkPhysicalDevice phsyical_device, uint32_t required_flags);
int select_present_queue_family(
    const struct lime_queue_family_table *table,
    VkPhysicalDevice phsyical_device, VkSurfaceKHR surface);
void destroy_queue_family_table(struct lime_queue_family_table *table);
