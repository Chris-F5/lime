#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

enum rule_type {
  RULE_TYPE_INSTANCE = 0,
  RULE_TYPE_DEBUG_MESSENGER = 1,
  RULE_TYPE_PHYSICAL_DEVICE = 3,
  RULE_TYPE_SURFACE = 4,
  RULE_TYPE_SURFACE_CAPABILITIES = 5,
  RULE_TYPE_QUEUE_FAMILY = 6,
  RULE_TYPE_QUEUE_FAMILY_GROUP = 7,
  RULE_TYPE_DEVICE = 8,
  RULE_TYPE_QUEUE = 9,
  RULE_TYPE_SWAPCHAIN = 10,
  RULE_TYPE_SWAPCHAIN_IMAGE_VIEWS = 11,
  RULE_TYPE_COMMAND_POOL = 12,
  RULE_TYPE_SHADER_MODULE = 13,
};

struct instance_conf {
  int validation_layers_requested;
};
struct instance_state {
  VkInstance instance;
  int validation_layers_enabled;
};

struct debug_messenger_conf {
  VkDebugUtilsMessageSeverityFlagsEXT severity_flags;
  VkDebugUtilsMessageTypeFlagsEXT type_flags;
  PFN_vkDebugUtilsMessengerCallbackEXT callback_func;
};
struct debug_messenger_state {
  VkDebugUtilsMessengerEXT debug_messenger;
};

struct physical_device_conf {
  const char *gpu_name;
};
struct physical_device_state {
  VkPhysicalDevice physical_device;
  VkPhysicalDeviceProperties properties;
};

struct surface_conf {
  GLFWwindow *window;
};
struct surface_state {
  VkSurfaceKHR surface;
};

struct surface_capabilities_state {
  VkSurfaceCapabilitiesKHR capabilities;
};

struct queue_family_conf {
  VkQueueFlags required_flags;
  unsigned char surface_support_required;
};
struct queue_family_state {
  uint32_t family_index;
};

struct queue_family_group_conf {
  int logical_family_count;
};
struct queue_family_group_state {
  int physical_family_count;
  uint32_t family_indices[];
};

struct command_pool_conf {
  VkCommandPoolCreateFlags flags;
};
struct command_pool_state {
  VkCommandPool command_pool;
};

struct device_conf {
  int extension_count;
  const char *const*extension_names;
  VkPhysicalDeviceFeatures features;
};
struct device_state {
  VkDevice device;
};

struct queue_state {
  VkQueue queue;
};

struct swapchain_conf {
  VkSwapchainCreateFlagsKHR flags;
  VkImageUsageFlags usage_flags;
  VkSurfaceFormatKHR surface_format;
  VkCompositeAlphaFlagBitsKHR composite_alpha;
  VkBool32 clipped;
  VkPresentModeKHR present_mode;
};
struct swapchain_state {
  VkSwapchainKHR swapchain;
  VkFormat image_format;
  uint32_t image_count;
  VkImage *images;
};

struct swapchain_image_views_conf {
};
struct swapchain_image_views_state {
  uint32_t image_count;
  VkImageView *image_views;
};

struct shader_module_conf {
  const char *file_name;
};
struct shader_module_state {
  VkShaderModule shader_module;
};

struct renderer {
  int rules_allocated, rule_count;
  long conf_memory_allocated, state_memory_allocated;
  int dependencies_allocated;
  long next_conf_offset, next_state_offset;
  int next_dependency_offset;
  int *rule_types;
  long *conf_offsets, *state_offsets;
  int *dependency_offsets;
  void *conf_memory, *state_memory;
  int *dependencies;
};

/* utils.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
char *vkresult_to_string(VkResult result);

/* renderer.c */
int add_rule(struct renderer *renderer, int type, size_t conf_size,
    size_t state_size);
void *get_rule_conf(const struct renderer *renderer, int rule);
void *get_rule_state(const struct renderer *renderer, int rule);
void *get_rule_dependency_state(const struct renderer *renderer, int rule,
    int dependency_index, int dependency_type);
void add_rule_dependency(struct renderer *renderer, int d);
void create_renderer(struct renderer *renderer, GLFWwindow* window);
void destroy_renderer(struct renderer *renderer);

/* rules.c */
extern const void (*rule_dispatch_funcs[])(struct renderer *renderer, int rule);
extern const void (*state_destroy_funcs[])(struct renderer *renderer, int rule);
int add_instance_rule(struct renderer *renderer, int request_validation_layers);
int add_debug_messenger_rule(struct renderer *renderer, int instance,
  VkDebugUtilsMessageSeverityFlagsEXT severity_flags,
  VkDebugUtilsMessageTypeFlagsEXT type_flags,
  PFN_vkDebugUtilsMessengerCallbackEXT callback_func);
int add_physical_device_rule(struct renderer *renderer, int instance,
    const char *gpu_name);
int add_window_surface_rule(struct renderer *renderer, int instance,
    GLFWwindow *window);
int add_surface_capabilities_rule(struct renderer *renderer, int physical_device,
    int surface);
int add_queue_family_rule(struct renderer *renderer, int physical_device,
    int surface, VkQueueFlags required_flags);
int add_queue_family_group_rule(struct renderer *renderer, int family_count,
    int *families);
int add_device_rule(struct renderer *renderer, int physical_device,
    int family_group, int extension_count, const char *const*extension_names);
int add_queue_rule(struct renderer *renderer, int device, int queue_family);
int add_swapchain_rule(struct renderer *renderer, int surface,
    int surface_capabilities, int device, int family_group,
    VkSwapchainCreateFlagsKHR flags, VkImageUsageFlags usage_flags,
    VkSurfaceFormatKHR surface_format,
    VkCompositeAlphaFlagBitsKHR composite_alpha, VkBool32 clipped,
    VkPresentModeKHR present_mode);
int add_swapchain_image_views_rule(struct renderer *renderer, int device,
    int swapchain);
int add_command_pool_rule(struct renderer *renderer, int device, int family,
    VkCommandPoolCreateFlags flags);
int add_shader_module_rule(struct renderer *renderer, int device,
    const char *file_name);
