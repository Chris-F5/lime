#version 450

layout(set = 0, binding = 0) uniform camera_uniform_buffer {
  mat4 old_model;
  mat4 view;
  mat4 proj;
};

layout(set = 1, binding = 0) uniform block_uniform_buffer {
  mat4 model;
  int block_scale;
};
layout(set = 1, binding = 1, r8ui) uniform uimage3D block_voxels;

layout(location = 0) in vec3 in_pos;

layout(location = 0) out vec4 out_color;

struct HitData {
  float distance;
  uint voxel;
  vec3 normal;
};

const float mat_specular = 1.0f;
const float mat_diffuse = 0.3f;
const float mat_ambient = 0.3f;
const float mat_shininess = 4.0f;

const vec3 light_dir = normalize(vec3(1.0f, -2.0f, 0.5f));
const float light_specular = 0.5f;
const float light_diffuse = 0.9f;

HitData
trace_ray(vec3 origin, vec3 dir)
{
  ivec3 step, out_of_bounds, pos;
  vec3 t_delta, t_max;
  float t;
  uvec4 img_dat;
  HitData hit, no_hit;

  pos = ivec3(floor(origin.x), floor(origin.y), floor(origin.z));
  no_hit.distance = 0.0f;
  no_hit.voxel = 0;
  no_hit.normal = vec3(0.0f, 0.0f, 0.0f);

  if (dir.x >= 0) {
    step.x = 1;
    out_of_bounds.x = block_scale;
    t_delta.x = 1 / dir.x;
    t_max.x = t_delta.x * (pos.x + 1 - origin.x);
    if (pos.x >= out_of_bounds.x) {
      return no_hit;
    }
  } else {
    step.x = -1;
    out_of_bounds.x = -1;
    t_delta.x = 1 / -dir.x;
    t_max.x = t_delta.x * (origin.x - pos.x);
    if (pos.x <= out_of_bounds.x) {
      return no_hit;
    }
  }

  if (dir.y >= 0) {
    step.y = 1;
    out_of_bounds.y = block_scale;
    t_delta.y = 1 / dir.y;
    t_max.y = t_delta.y * (pos.y + 1 - origin.y);
    if (pos.y >= out_of_bounds.y) {
      return no_hit;
    }
  } else {
    step.y = -1;
    out_of_bounds.y = -1;
    t_delta.y = 1 / -dir.y;
    t_max.y = t_delta.y * (origin.y - pos.y);
    if (pos.y <= out_of_bounds.y) {
      return no_hit;
    }
  }

  if (dir.z >= 0) {
    step.z = 1;
    out_of_bounds.z = block_scale;
    t_delta.z = 1 / dir.z;
    t_max.z = t_delta.z * (pos.z + 1 - origin.z);
    if (pos.z >= out_of_bounds.z) {
      return no_hit;
    }
  } else {
    step.z = -1;
    out_of_bounds.z = -1;
    t_delta.z = 1 / -dir.z;
    t_max.z = t_delta.z * (origin.z - pos.z);
    if (pos.z <= out_of_bounds.z) {
      return no_hit;
    }
  }

  t = 0;
  while (true) {
    if(t_max.x < t_max.y) {
      if (t_max.x < t_max.z) {
        pos.x += step.x;
        t = t_max.x;
        if (pos.x == out_of_bounds.x) {
          break;
        }
        t_max.x += t_delta.x;
        hit.normal = vec3(-step.x, 0.0, 0.0);
      } else {
        pos.z += step.z;
        t = t_max.z;
        if (pos.z == out_of_bounds.z) {
          break;
        }
        t_max.z += t_delta.z;
        hit.normal = vec3(0.0, 0.0, -step.z);
      }
    } else {
      if (t_max.y < t_max.z) {
        pos.y += step.y;
        t = t_max.y;
        if (pos.y == out_of_bounds.y) {
          break;
        }
        t_max.y += t_delta.y;
        hit.normal = vec3(0.0, -step.y, 0);
      } else {
        pos.z += step.z;
        t = t_max.z;
        if (pos.z == out_of_bounds.z) {
          break;
        }
        t_max.z += t_delta.z;
        hit.normal = vec3(0.0, 0.0, -step.z);
      }
    }

    img_dat = imageLoad(block_voxels, pos);
    if (img_dat.x != 0) {
      hit.distance = t;
      hit.voxel = img_dat.x;
      return hit;
    }
  }
}

float
compute_illumination(vec3 normal, vec3 viewer)
{
  vec3 reflection;
  float illumination;
  reflection = 2 * normal * dot(normal, -light_dir) + light_dir;
  illumination
    = mat_diffuse * light_diffuse * dot(normal, -light_dir)
    + mat_specular * light_specular * pow(dot(reflection, viewer), mat_shininess)
    + mat_ambient;
  return illumination;
}

float
distance_to_depth(float distance)
{
  return proj[2][2] + proj[3][2] / distance;
}

void
main()
{
  vec3 cam_pos, cam_dir;
  vec3 ray_dir;
  HitData hit;
  float illumination;

  cam_pos = vec3(inverse(model) * inverse(view) * vec4(0.0f, 0.0f, 0.0f, 1.0f));
  cam_dir = normalize(vec3(inverse(view)[2]));
  ray_dir = normalize(in_pos - cam_pos);
  hit = trace_ray(cam_pos * 16, ray_dir);
  if (hit.voxel != 0) {
    illumination = compute_illumination(hit.normal, -ray_dir);
    out_color = vec4(1.0f, 0.0f, 0.0f, 1.0f) * illumination;
    gl_FragDepth = distance_to_depth(hit.distance / block_scale * dot(cam_dir, ray_dir));
  } else {
    out_color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    gl_FragDepth = 1.0f;
  }
}
