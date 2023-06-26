#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

struct lime_queue_family_table {
  int count;
  int *hardware_id;
  VkQueueFamilyProperties *properties;
};

struct lime_hardware_table {
  int count;
  VkPhysicalDevice *devices;
  VkSurfaceCapabilitiesKHR *surface_capabilities;
};

struct lime_queue_table {
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

struct lime_pipeline_table {
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

struct lime_renderer {
  int validation_layers_enabled;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkSurfaceKHR surface;

  VkSurfaceFormatKHR surfaceFormat;
  VkPresentModeKHR presentMode;
  VkFormat depthImageFormat;

  struct lime_hardware_table hardware;
  struct lime_queue_family_table queue_families;
  struct lime_device_table devices;
  struct lime_command_pool_table command_pools;
  struct lime_queue_table render_queues;
  struct lime_swapchain_table swapchains;
};

/* utils.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
char *vkresult_to_string(VkResult result);

/* renderer.c */
void create_renderer(struct lime_renderer *renderer, GLFWwindow* window);
void destroy_renderer(struct lime_renderer *renderer);
