/*
struct render_instance {
  int validation_layers_enabled;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
};

struct render_surface {
  VkSurfaceKHR surface;
};

struct render_hardware {
  VkPhysicalDevice physical;
};

struct render_device {
  VkDevice logical;
  int graphics_family_index, present_family_index;
  VkFormat depth_image_format;
  VkSurfaceCapabilitiesKHR initial_surface_capabilities;
  VkSurfaceFormatKHR surface_format;
  VkPresentModeKHR present_mode;
  VkQueue graphics_queue;
  VkQueue present_queue;
  VkCommandPool command_pool;
  VkCommandPool transient_command_pool;
};

struct render_swapchain {
  VkExtent2D extent;
  VkFormat image_format;
  uint32_t length;
  VkSwapchainKHR swapchain;
  VkImage* images;
  VkImageView* image_views;
};
*/

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

void create_render_instance(struct render_instance *instance,
    int validation_layers_enabled);
void create_window(struct render_surface *surface);
void select_render_device(struct render_device *device,
    const struct render_instance *instance,
    const struct render_surface *surface);
void create_swapchain(struct render_swapchain *swapchain,
    const struct render_device *device, const struct render_surface *surface);
