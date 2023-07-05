#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

static void add_rules(struct renderer *renderer);
static void dispatch(struct renderer *renderer);
static int get_dependency_count(const struct renderer *renderer, int rule);

static void
add_rules(struct renderer *renderer)
{
  int instance, physical_device;
  instance = add_instance_rule(renderer);
  physical_device = add_physical_device_rule(renderer, instance, "GPU NAME");
}


static void
dispatch(struct renderer *renderer)
{
  int i;
  struct rule *rule;
  for (i = 0; i < renderer->rule_count; i++) {
    rule = get_rule(renderer, i);
    switch(rule->type) {
    case RULE_TYPE_INSTANCE:
      dispatch_instance_rule(renderer, i);
      break;
    case RULE_TYPE_PHYSICAL_DEVICE:
      dispatch_physical_device_rule(renderer, i);
      break;
    default:
      fprintf(stderr, "unrecognised rule type (%d)\n", rule->type);
      exit(1);
    }
  }
}

static void
destroy_state(struct renderer *renderer)
{
  int i;
  struct rule *rule;
  void *state;
  for (i = renderer->rule_count - 1; i >= 0; i--) {
    rule = get_rule(renderer, i);
    state = get_state(renderer, i);
    switch(rule->type) {
    case RULE_TYPE_INSTANCE:
      destroy_instance(renderer, i);
      break;
    case RULE_TYPE_PHYSICAL_DEVICE:
      destroy_physical_device(renderer, i);
      break;
    default:
      fprintf(stderr, "unrecognised rule type (%d)\n", rule->type);
      exit(1);
    }
  }
}

static int
get_dependency_count(const struct renderer *renderer, int rule)
{
  assert(rule >= 0);
  assert(rule < renderer->rule_count);
  return (
      rule == renderer->rule_count - 1
      ? renderer->next_dependency_offset
      : renderer->dependency_offsets[rule + 1]
      ) - renderer->dependency_offsets[rule];
}

int
add_rule(struct renderer *renderer, size_t rule_size, size_t state_size)
{
  int index;
  index = renderer->rule_count++;
  if (renderer->rule_count > renderer->rules_allocated) {
    renderer->rules_allocated += 64;
    renderer->rule_offsets = xrealloc(renderer->rule_offsets,
        renderer->rules_allocated * sizeof(long));
    renderer->state_offsets = xrealloc(renderer->state_offsets,
        renderer->rules_allocated * sizeof(long));
    renderer->dependency_offsets = xrealloc(renderer->dependency_offsets,
        renderer->rules_allocated * sizeof(long));
  }
  if (renderer->next_rule_offset + rule_size > renderer->rule_memory_allocated) {
    renderer->rule_memory_allocated
      = renderer->next_rule_offset + rule_size + 1024 * 4;
    renderer->rule_memory = xrealloc(renderer->rule_memory,
        renderer->rule_memory_allocated);
  }
  if (renderer->next_state_offset + state_size > renderer->state_memory_allocated) {
    renderer->state_memory_allocated
      = renderer->next_state_offset + state_size + 1024 * 4;
    renderer->state_memory = xrealloc(renderer->state_memory,
        renderer->state_memory_allocated);
  }
  renderer->rule_offsets[index] = renderer->next_rule_offset;
  renderer->next_rule_offset += rule_size;
  renderer->state_offsets[index] = renderer->next_state_offset;
  renderer->next_state_offset += state_size;
  renderer->dependency_offsets[index] = renderer->next_dependency_offset;
  return index;
}

void *
get_rule(const struct renderer *renderer, int rule)
{
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  return renderer->rule_memory + renderer->rule_offsets[rule];
}

void *
get_state(const struct renderer *renderer, int rule)
{
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  return renderer->state_memory + renderer->state_offsets[rule];
}

void *
get_dependency(struct renderer *renderer, int rule, int n)
{
  int dependency_rule;
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  if (n < 0 || n > get_dependency_count(renderer, rule)) {
    fprintf(stderr, "rule does not have dependency (%d)\n", n);
    exit(1);
  }
  dependency_rule = renderer->dependencies[renderer->dependency_offsets[rule] + n];
  return renderer->state_memory + renderer->state_offsets[dependency_rule];
}

void
add_dependency(struct renderer *renderer, int d)
{
  if (renderer->next_dependency_offset == renderer->dependencies_allocated) {
    renderer->dependencies_allocated += 128;
    renderer->dependencies = xrealloc(renderer->dependencies,
        renderer->dependencies_allocated * sizeof(int));
  }
  renderer->dependencies[renderer->next_dependency_offset++] = d;
}

void
create_renderer(struct renderer *renderer, GLFWwindow* window)
{
  renderer->rule_count = 0;
  renderer->rules_allocated = 0;
  renderer->rule_offsets = NULL;
  renderer->state_offsets = NULL;
  renderer->dependency_offsets = NULL;
  renderer->next_rule_offset = 0;
  renderer->next_state_offset = 0;
  renderer->next_dependency_offset = 0;
  renderer->rule_memory_allocated = 0;
  renderer->state_memory_allocated = 0;
  renderer->dependencies_allocated = 0;
  renderer->rule_memory = NULL;
  renderer->state_memory = NULL;
  renderer->dependencies = NULL;
  add_rules(renderer);
  dispatch(renderer);
}

void
destroy_renderer(struct renderer *renderer)
{
  destroy_state(renderer);
  free(renderer->rule_offsets);
  free(renderer->state_offsets);
  free(renderer->dependency_offsets);
  free(renderer->rule_memory);
  free(renderer->state_memory);
  free(renderer->dependencies);
}
