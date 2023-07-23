#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "obj_types.h"
#include "obj.h"
#include "utils.h"

char line_buffer[256];

void
load_wavefront_obj(struct wavefront_obj *obj, const char *fname)
{
  FILE *file;
  int pos_index, uv_index, normal_index, face_index;
  file = fopen(fname, "r");
  if (file == NULL) {
    fprintf(stderr, "Failed to open obj file '%s'.\n", fname);
    exit(1);
  }
  obj->position_count = obj->uv_count = obj->normal_count = 0;
  obj->face_count = 0;
  while (fgets(line_buffer, sizeof(line_buffer), file)) {
    if (line_buffer[0] == 'v' && line_buffer[1] == ' ')
      obj->position_count++;
    if (line_buffer[0] == 'v' && line_buffer[1] == 't' && line_buffer[2] == ' ')
      obj->uv_count++;
    if (line_buffer[0] == 'v' && line_buffer[1] == 'n' && line_buffer[2] == ' ')
      obj->normal_count++;
    if (line_buffer[0] == 'f' && line_buffer[1] == ' ')
      obj->face_count++;
  }
  if (fseek(file, 0, SEEK_SET)) {
    perror("Failed to seek in obj file");
    exit(1);
  }
  assert(obj->position_count);
  obj->positions = xmalloc(obj->position_count * 3 * sizeof(float));
  obj->position_faces = xmalloc(obj->face_count * 3 * sizeof(int));
  if (obj->uv_count) {
    obj->uvs = xmalloc(obj->uv_count * 2 * sizeof(float));
    obj->uv_faces = xmalloc(obj->face_count * 3 * sizeof(int));
  } else {
    obj->uvs = NULL;
    obj->uv_faces = NULL;
  }
  if (obj->normal_count) {
    obj->normals = xmalloc(obj->normal_count * 3 * sizeof(float));
    obj->normal_faces = xmalloc(obj->face_count * 3 * sizeof(int));
  } else {
    obj->normals = NULL;
    obj->normal_faces = NULL;
  }
  pos_index = uv_index = normal_index = face_index = 0;
  while (fgets(line_buffer, sizeof(line_buffer), file)) {
    if (line_buffer[0] == 'v' && line_buffer[1] == ' ') {
      assert(pos_index < obj->position_count);
      if (sscanf(line_buffer, "v %f %f %f\n",
            &obj->positions[pos_index * 3],
            &obj->positions[pos_index * 3 + 1],
            &obj->positions[pos_index * 3 + 2]) != 3) {
        fprintf(stderr, "Error parsing obj file '%s' '%s'.\n", fname, line_buffer);
        exit(1);
      }
      pos_index++;
    } else if (line_buffer[0] == 'v' && line_buffer[1] == 't' && line_buffer[2] == ' ') {
      assert(uv_index < obj->uv_count);
      if (sscanf(line_buffer, "vt %f %f\n",
            &obj->uvs[uv_index * 2],
            &obj->uvs[uv_index * 2 + 1]) != 2) {
        fprintf(stderr, "Error parsing obj file '%s' '%s'.\n", fname, line_buffer);
        exit(1);
      }
      uv_index++;
    } else if (line_buffer[0] == 'v' && line_buffer[1] == 'n' && line_buffer[2] == ' ') {
      assert(normal_index < obj->normal_count);
      if (sscanf(line_buffer, "vn %f %f %f\n",
            &obj->normals[normal_index * 3],
            &obj->normals[normal_index * 3 + 1],
            &obj->normals[normal_index * 3 + 2]) != 3) {
        fprintf(stderr, "Error parsing obj file '%s' '%s'.\n", fname, line_buffer);
        exit(1);
      }
      normal_index++;
    } else if (line_buffer[0] == 'f' && line_buffer[1] == ' ') {
      assert(face_index < obj->face_count);
      if (sscanf(line_buffer, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
            &obj->position_faces[face_index * 3],
            &obj->uv_faces[face_index * 3],
            &obj->normal_faces[face_index * 3],
            &obj->position_faces[face_index * 3 + 1],
            &obj->uv_faces[face_index * 3 + 1],
            &obj->normal_faces[face_index * 3 + 1],
            &obj->position_faces[face_index * 3 + 2],
            &obj->uv_faces[face_index * 3 + 2],
            &obj->normal_faces[face_index * 3 + 2]) != 9) {
        fprintf(stderr, "Error parsing obj file '%s' '%s'.\n", fname, line_buffer);
        exit(1);
      }
      obj->position_faces[face_index * 3]--;
      obj->position_faces[face_index * 3 + 1]--;
      obj->position_faces[face_index * 3 + 2]--;
      obj->uv_faces[face_index * 3]--;
      obj->uv_faces[face_index * 3 + 1]--;
      obj->uv_faces[face_index * 3 + 2]--;
      obj->normal_faces[face_index * 3]--;
      obj->normal_faces[face_index * 3 + 1]--;
      obj->normal_faces[face_index * 3 + 2]--;
      assert(obj->position_faces[face_index * 3] >= 0 && obj->position_faces[face_index * 3] < obj->position_count);
      assert(obj->position_faces[face_index * 3 + 1] >= 0 && obj->position_faces[face_index * 3 + 1] < obj->position_count);
      assert(obj->position_faces[face_index * 3 + 1] >= 0 && obj->position_faces[face_index * 3 + 1] < obj->position_count);
      if (obj->uv_faces) {
        assert(obj->uv_faces[face_index * 3] >= 0 && obj->uv_faces[face_index * 3] < obj->uv_count);
        assert(obj->uv_faces[face_index * 3 + 1] >= 0 && obj->uv_faces[face_index * 3 + 1] < obj->uv_count);
        assert(obj->uv_faces[face_index * 3 + 2] >= 0 && obj->uv_faces[face_index * 3 + 2] < obj->uv_count);
      }
      if (obj->normal_faces) {
        assert(obj->normal_faces[face_index * 3] >= 0 && obj->normal_faces[face_index * 3] < obj->normal_count);
        assert(obj->normal_faces[face_index * 3 + 1] >= 0 && obj->normal_faces[face_index * 3 + 1] < obj->normal_count);
        assert(obj->normal_faces[face_index * 3 + 2] >= 0 && obj->normal_faces[face_index * 3 + 2] < obj->normal_count);
      }
      face_index++;
    }
  }
  assert(pos_index == obj->position_count);
  assert(uv_index == obj->uv_count);
  assert(normal_index == obj->normal_count);
  assert(face_index == obj->face_count);
  if (ferror(file)) {
    fprintf(stderr, "Error while reading obj file '%s'.\n", fname);
    exit(1);
  }
  fclose(file);
}

void
destroy_wavefront_obj(struct wavefront_obj *obj)
{
  free(obj->positions);
  free(obj->uvs);
  free(obj->normals);
  free(obj->position_faces);
  free(obj->uv_faces);
  free(obj->normal_faces);
}

void
wavefront_to_indexed_vertex_obj(struct indexed_vertex_obj *ivo,
    const struct wavefront_obj *wavefront)
{
  int vert, next_index;
  ivo->vertices = xmalloc(wavefront->face_count * 3 * sizeof(struct vertex));
  ivo->indices = xmalloc(wavefront->face_count * 3 * sizeof(int));
  vert = next_index = 0;
  /* TODO: Reuse index for identical vertices. */
  for (vert = 0; vert < wavefront->face_count * 3; vert++) {
    memcpy(&ivo->vertices[next_index].pos,
        &wavefront->positions[wavefront->position_faces[vert] * 3],
        3 * sizeof(float));
    if (wavefront->uv_faces) {
      ivo->vertices[next_index].uv[0] = wavefront->uvs[wavefront->uv_faces[vert] * 2];
      ivo->vertices[next_index].uv[1] = 1.0f - wavefront->uvs[wavefront->uv_faces[vert] * 2 + 1];
    } else {
      ivo->vertices[next_index].uv[0] = 0.0f;
      ivo->vertices[next_index].uv[1] = 0.0f;
    }
    if (wavefront->normal_faces) {
      memcpy(&ivo->vertices[next_index].normal,
          &wavefront->normals[wavefront->normal_faces[vert] * 3],
          3 * sizeof(float));
    } else {
      ivo->vertices[next_index].normal[0] = 0.0f;
      ivo->vertices[next_index].normal[1] = 0.0f;
      ivo->vertices[next_index].normal[2] = 0.0f;
    }
    ivo->indices[vert] = next_index++;
  }
  ivo->vertex_count = next_index;
  ivo->index_count = wavefront->face_count * 3;
}

void
destroy_indexed_vertex_obj(struct indexed_vertex_obj *ivo)
{
  free(ivo->vertices);
  free(ivo->indices);
}
