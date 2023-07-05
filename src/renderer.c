#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "lime.h"

static void configure_rules(struct renderer *renderer);
static void dispatch_rules(struct renderer *renderer);
static void destroy_state(struct renderer *renderer);
static int get_rule_dependency_count(const struct renderer *renderer, int rule);

static void
configure_rules(struct renderer *renderer)
{
  int instance, physical_device;
  instance = add_instance_rule(renderer);
  physical_device = add_physical_device_rule(renderer, instance, "GPU NAME");
}


static void
dispatch_rules(struct renderer *renderer)
{
  int rule, type;
  for (rule = 0; rule < renderer->rule_count; rule++) {
    type = renderer->rule_types[rule];
    switch(type) {
    case RULE_TYPE_INSTANCE:
      dispatch_instance_rule(renderer, rule);
      break;
    case RULE_TYPE_PHYSICAL_DEVICE:
      dispatch_physical_device_rule(renderer, rule);
      break;
    default:
      fprintf(stderr, "unrecognised rule type (%d)\n", type);
      exit(1);
    }
  }
}

static void
destroy_state(struct renderer *renderer)
{
  int rule, type;
  for (rule = renderer->rule_count - 1; rule >= 0; rule--) {
    type = renderer->rule_types[rule];
    switch(type) {
    case RULE_TYPE_INSTANCE:
      destroy_instance_state(renderer, rule);
      break;
    case RULE_TYPE_PHYSICAL_DEVICE:
      destroy_physical_device_state(renderer, rule);
      break;
    default:
      fprintf(stderr, "unrecognised rule type (%d)\n", type);
      exit(1);
    }
  }
}

static int
get_rule_dependency_count(const struct renderer *renderer, int rule)
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
add_rule(struct renderer *renderer, int type, size_t conf_size, size_t state_size)
{
  int rule;
  rule = renderer->rule_count++;
  if (renderer->rule_count > renderer->rules_allocated) {
    renderer->rules_allocated += 64;
    renderer->rule_types = xrealloc(renderer->rule_types,
        renderer->rules_allocated * sizeof(int));
    renderer->conf_offsets = xrealloc(renderer->conf_offsets,
        renderer->rules_allocated * sizeof(long));
    renderer->state_offsets = xrealloc(renderer->state_offsets,
        renderer->rules_allocated * sizeof(long));
    renderer->dependency_offsets = xrealloc(renderer->dependency_offsets,
        renderer->rules_allocated * sizeof(int));
  }
  if (renderer->next_conf_offset + conf_size > renderer->conf_memory_allocated) {
    renderer->conf_memory_allocated
      = renderer->next_conf_offset + conf_size + 1024 * 4;
    renderer->conf_memory = xrealloc(renderer->conf_memory,
        renderer->conf_memory_allocated);
  }
  if (renderer->next_state_offset + state_size > renderer->state_memory_allocated) {
    renderer->state_memory_allocated
      = renderer->next_state_offset + state_size + 1024 * 4;
    renderer->state_memory = xrealloc(renderer->state_memory,
        renderer->state_memory_allocated);
  }
  renderer->rule_types[rule] = type;
  renderer->conf_offsets[rule] = renderer->next_conf_offset;
  renderer->next_conf_offset += conf_size;
  renderer->state_offsets[rule] = renderer->next_state_offset;
  renderer->next_state_offset += state_size;
  renderer->dependency_offsets[rule] = renderer->next_dependency_offset;
  return rule;
}

void *
get_rule_conf(const struct renderer *renderer, int rule)
{
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  return renderer->conf_memory + renderer->conf_offsets[rule];
}

void *
get_rule_state(const struct renderer *renderer, int rule)
{
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  return renderer->state_memory + renderer->state_offsets[rule];
}

void *
get_rule_dependency_state(const struct renderer *renderer, int rule,
    int dependency_index, int dependency_type)
{
  int dependency_rule;
  if (rule < 0 || rule >= renderer->rule_count) {
    fprintf(stderr, "rule out of range (%d)\n", rule);
    exit(1);
  }
  if (dependency_index < 0
      || dependency_index > get_rule_dependency_count(renderer, rule)) {
    fprintf(stderr, "rule does not have dependency (%d)\n", dependency_index);
    exit(1);
  }
  dependency_rule = renderer->dependencies[
    renderer->dependency_offsets[rule] + dependency_index];
  if (renderer->rule_types[dependency_rule] != dependency_type) {
    fprintf(stderr, "unexpected dependency on rule type %d:\n\
  dependency index %d\n\
  expected type %d\n\
  actual type %d\n",
  renderer->rule_types[rule], dependency_index, dependency_type,
  renderer->rule_types[dependency_rule]);
    exit(1);
  }
  return renderer->state_memory + renderer->state_offsets[dependency_rule];
}

void
add_rule_dependency(struct renderer *renderer, int d)
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
  renderer->rules_allocated = 0;
  renderer->rule_count = 0;
  renderer->conf_memory_allocated = 0;
  renderer->state_memory_allocated = 0;
  renderer->dependencies_allocated = 0;
  renderer->next_conf_offset = 0;
  renderer->next_state_offset = 0;
  renderer->next_dependency_offset = 0;
  renderer->rule_types = NULL;
  renderer->conf_offsets = NULL;
  renderer->state_offsets = NULL;
  renderer->dependency_offsets = NULL;
  renderer->conf_memory = NULL;
  renderer->state_memory = NULL;
  renderer->dependencies = NULL;
  configure_rules(renderer);
  dispatch_rules(renderer);
}

void
destroy_renderer(struct renderer *renderer)
{
  destroy_state(renderer);
  free(renderer->rule_types);
  free(renderer->conf_offsets);
  free(renderer->state_offsets);
  free(renderer->dependency_offsets);
  free(renderer->conf_memory);
  free(renderer->state_memory);
  free(renderer->dependencies);
}
