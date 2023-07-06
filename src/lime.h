#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

enum rule_type {
  RULE_TYPE_INSTANCE = 0,
  RULE_TYPE_DEBUG_MESSENGER = 1,
  RULE_TYPE_PHYSICAL_DEVICE = 3,
  RULE_TYPE_SURFACE = 4,
  RULE_TYPE_QUEUE_FAMILY = 5,
  RULE_TYPE_DEVICE = 6,
  RULE_TYPE_QUEUE = 7,
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

struct queue_family_conf {
  VkQueueFlags required_flags;
  unsigned char surface_support_required;
};
struct queue_family_state {
  uint32_t family_index;
};

struct device_conf {
  int family_count;
  int extension_count;
  const char *const*extension_names;
  VkPhysicalDeviceFeatures features;
};
struct device_state {
  VkDevice device;
};

struct queue_conf {
};
struct queue_state {
  VkQueue queue;
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
extern void (*rule_dispatch_funcs[])(struct renderer *renderer, int rule);
extern void (*state_destroy_funcs[])(struct renderer *renderer, int rule);
int add_instance_rule(struct renderer *renderer, int request_validation_layers);
int add_debug_messenger_rule(struct renderer *renderer, int instance,
  VkDebugUtilsMessageSeverityFlagsEXT severity_flags,
  VkDebugUtilsMessageTypeFlagsEXT type_flags,
  PFN_vkDebugUtilsMessengerCallbackEXT callback_func);
int add_physical_device_rule(struct renderer *renderer, int instance,
    const char *gpu_name);
int add_window_surface_rule(struct renderer *renderer, int instance,
    GLFWwindow *window);
int add_queue_family_rule(struct renderer *renderer, int physical_device,
    int surface, VkQueueFlags required_flags);
int add_device_rule(struct renderer *renderer, int physical_device,
    int family_count, int *families, int extension_count,
    const char *const*extension_names);
int create_queue_rule(struct renderer *renderer, int device, int queue_family);
