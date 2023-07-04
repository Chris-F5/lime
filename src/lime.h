#define PRINT_VK_ERROR(err, string) { \
  fprintf(stderr, "vulkan error at %s:%d '%s':%s\n", __FILE__, __LINE__, \
      string, vkresult_to_string(err)); \
  }

enum rule_type {
  RULE_TYPE_INSTANCE = 0,
};

struct tuple {
  long memory_allocated, next_offset;
  int elements_allocated, element_count;
  void *memory;
  long *offsets;
};

struct instance_rule {
  int type;
  int validation_layers_enabled;
};
struct instance_state {
  VkInstance instance;
};

struct rule {
  int type;
  char _[];
};

struct renderer {
  struct tuple rules;
  struct tuple state;
};

/* tuple.c */
void tuple_init(struct tuple *tuple, size_t memory_allocate, int elements_allocate);
int tuple_add(struct tuple *tuple, size_t size);
void *tuple_get(struct tuple *tuple, int element);
void tuple_free(struct tuple *tuple);

/* utils.c */
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
char *vkresult_to_string(VkResult result);

/* renderer.c */
void create_renderer(struct renderer *renderer, GLFWwindow* window);
void destroy_renderer(struct renderer *renderer);
