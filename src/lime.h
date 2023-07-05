#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

enum rule_type {
  RULE_TYPE_INSTANCE = 0,
  RULE_TYPE_PHYSICAL_DEVICE = 1,
};

struct instance_conf {
  int validation_layers_enabled;
};
struct instance_state {
  VkInstance instance;
};

struct physical_device_conf {
  const char *gpu_name;
};
struct physical_device_state {
  VkPhysicalDevice physical_device;
  VkPhysicalDeviceProperties properties;
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
int add_rule(struct renderer *renderer, int type, size_t conf_size, size_t state_size);
void *get_rule_conf(const struct renderer *renderer, int rule);
void *get_rule_state(const struct renderer *renderer, int rule);
void *get_rule_dependency_state(const struct renderer *renderer, int rule,
    int dependency_index, int dependency_type);
void add_rule_dependency(struct renderer *renderer, int d);
void create_renderer(struct renderer *renderer, GLFWwindow* window);
void destroy_renderer(struct renderer *renderer);

/* device.c */
int add_instance_rule(struct renderer *renderer);
void dispatch_instance_rule(struct renderer *renderer, int rule_index);
void destroy_instance_state(struct renderer *renderer, int rule_index);
int add_physical_device_rule(struct renderer *renderer, int instance,
    const char *gpu_name);
void dispatch_physical_device_rule(struct renderer *renderer, int rule_index);
void destroy_physical_device_state(struct renderer *renderer, int rule_index);
