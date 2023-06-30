#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

/*
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

struct lime_command_buffer_table {
  int count, allocated;
  int *device_id;
  VkCommandBuffer *command_buffer;
};
*/

struct lime_physical_device_table {
  int count;
  VkPhysicalDevice *physical_device;
  VkPhysicalDeviceProperties *properties;
};

struct lime_queue_family_table {
  int count, allocated;
  VkPhysicalDevice *physical_device;
  int *family_index;
  VkQueueFamilyProperties *properties;
};

struct lime_logical_device_table {
  int count, allocated;
  VkDevice *logical_device;
  VkPhysicalDevice *physical_device;
};

struct lime_queue_table {
  int count, allocated;
  VkDevice *logical_device;
  int *family_index;
  int *queue_index;
  VkQueue *queue;
};

struct lime_command_pool_table {
  int count, allocated;
  VkCommandPool *command_pool;
  VkDevice *logical_device;
  int *family_index;
};

struct lime_swapchain_image_table {
  int count, allocated;
  VkImage *image;
  VkImageView *image_view;
  VkSwapchainKHR *swapchain;
};

struct lime_swapchain_table {
  int count, allocated;
  VkSwapchainKHR *swapchain;
  VkDevice *logical_device;
  VkSurfaceFormatKHR *surface_format;
  VkExtent2D *extent;
};

struct lime_shader_module_table {
  int count, allocated;
  VkShaderModule *shader_module;
  VkDevice *logical_device;
  const char **file_name;
};

struct lime_renderer {
  int validation_layers_enabled;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkSurfaceKHR surface;

  struct lime_physical_device_table physical_devices;
  struct lime_queue_family_table queue_families;
  struct lime_logical_device_table logical_devices;
  struct lime_queue_table device_queues;
  struct lime_command_pool_table command_pools;
  struct lime_swapchain_table swapchains;
  struct lime_swapchain_image_table swapchain_images;
  struct lime_shader_module_table shader_modules;
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
void destroy_queue_family_table(struct lime_queue_family_table *table);
int check_physical_device_extension_support(VkPhysicalDevice physical_device,
    int required_extension_count, const char * const *required_extension_names);
int select_queue_family_with_flags(const struct lime_queue_family_table *table,
    VkPhysicalDevice phsyical_device, uint32_t required_flags);
int select_queue_family_with_present_support(
    const struct lime_queue_family_table *table,
    VkPhysicalDevice phsyical_device, VkSurfaceKHR surface);

/* logical_device.c */
void create_logical_device_table(struct lime_logical_device_table *table);
void destroy_logical_device_table(struct lime_logical_device_table *table);
void create_queue_table(struct lime_queue_table *table);
void destroy_queue_table(struct lime_queue_table *table);
void create_command_pool_table(struct lime_command_pool_table *table);
void destroy_command_pool_table(struct lime_command_pool_table *table);
VkDevice create_logical_device(
    struct lime_logical_device_table *logical_device_table,
    struct lime_queue_table *queue_table, VkPhysicalDevice physical_device,
    int queue_family_count, const int *queue_families, int extension_count,
    const char * const* extension_names, VkPhysicalDeviceFeatures features);
int get_logical_device_table_index(const struct lime_logical_device_table *table,
    VkDevice logical_device);
void create_command_pool(struct lime_command_pool_table *table,
    VkDevice logical_device, int family_index, VkCommandPoolCreateFlags flags);

/* swapchain.c */
void create_swapchain_table(struct lime_swapchain_table *table);
void destroy_swapchain_table(struct lime_swapchain_table *swapchain_table);
void create_swapchain_image_table(struct lime_swapchain_image_table *table);
void destroy_swapchain_image_table(
    struct lime_swapchain_image_table *swapchain_image_table,
    const struct lime_swapchain_table *swapchain_table);
int find_swapchain_table_index(const struct lime_swapchain_table *table,
    VkSwapchainKHR swapchain);
void create_swapchain(struct lime_swapchain_table *swapchain_table,
    struct lime_swapchain_image_table *image_table,
    VkPhysicalDevice physical_device, VkSurfaceKHR surface,
    VkDevice logical_device, VkSwapchainCreateFlagsKHR flags,
    VkSurfaceFormatKHR surface_format, VkImageUsageFlags usage_flags,
    VkPresentModeKHR present_mode, int queue_family_count,
    const uint32_t *queue_family_indices, VkBool32 clipped,
    VkCompositeAlphaFlagBitsKHR composite_alpha);

/* shader_module.c */
void create_shader_module_table(struct lime_shader_module_table *table);
void destroy_shader_module_table(struct lime_shader_module_table *table);
VkShaderModule create_shader_module(struct lime_shader_module_table *table,
    VkDevice logical_device, const char *file_name);
