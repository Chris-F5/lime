#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

enum rule_type {
  RULE_TYPE_INSTANCE = 0,
  RULE_TYPE_PHYSICAL_DEVICE = 1,
};

struct instance_rule {
  int type;
  int validation_layers_enabled;
};
struct instance_state {
  VkInstance instance;
};

struct physical_device_rule {
  int type;
  const char *gpu_name;
};
struct physical_device_state {
  VkPhysicalDevice physical_device;
  VkPhysicalDeviceProperties properties;
};

struct rule {
  int type;
  char _[];
};

struct renderer {
  int rules_allocated, rule_count;
  long rule_memory_allocated, next_rule_offset;
  long state_memory_allocated, next_state_offset;
  int dependencies_allocated, next_dependency_offset;
  long *rule_offsets, *state_offsets;
  int *dependency_offsets;
  void *rule_memory, *state_memory;
  int *dependencies;
};

/* utils.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
char *vkresult_to_string(VkResult result);

/* renderer.c */
int add_rule(struct renderer *renderer, size_t rule_size, size_t state_size);
void *get_rule(const struct renderer *renderer, int rule);
void *get_state(const struct renderer *renderer, int rule);
void *get_dependency(struct renderer *renderer, int rule, int n);
void add_dependency(struct renderer *renderer, int d);
void create_renderer(struct renderer *renderer, GLFWwindow* window);
void destroy_renderer(struct renderer *renderer);

/* device.c */
int add_instance_rule(struct renderer *renderer);
void dispatch_instance_rule(struct renderer *renderer, int rule_index);
void destroy_instance(struct renderer *renderer, int rule_index);
int add_physical_device_rule(struct renderer *renderer, int instance,
    const char *gpu_name);
void dispatch_physical_device_rule(struct renderer *renderer, int rule_index);
void destroy_physical_device(struct renderer *renderer, int rule_index);
