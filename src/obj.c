#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "obj.h"
#include "utils.h"

char line_buffer[256];

void
parse_obj(const char *fname, int *vertex_count, int *face_count, float **vertex_positions,
    uint32_t **vertex_indices)
{
  FILE *file;
  int vertex_index, face_index;
  file = fopen(fname, "r");
  if (file == NULL) {
    fprintf(stderr, "Failed to open obj file '%s'.\n", fname);
    exit(1);
  }
  *vertex_count = 0;
  *face_count = 0;
  while (fgets(line_buffer, sizeof(line_buffer), file)) {
    if (line_buffer[0] == 'v' && line_buffer[1] == ' ')
      (*vertex_count)++;
    if (line_buffer[0] == 'f' && line_buffer[1] == ' ')
      (*face_count)++;
  }
  if (fseek(file, 0, SEEK_SET)) {
    perror("Failed to seek in obj file");
    exit(1);
  }
  *vertex_positions = xmalloc(*vertex_count * 4 * sizeof(float));
  *vertex_indices = xmalloc(*face_count * 3 * sizeof(uint32_t));
  vertex_index = 0;
  face_index = 0;
  while (fgets(line_buffer, sizeof(line_buffer), file)) {
    if (line_buffer[0] == 'v' && line_buffer[1] == ' ') {
      assert(vertex_index < *vertex_count);
      if (sscanf(line_buffer, "v %f %f %f\n",
            &(*vertex_positions)[vertex_index * 4],
            &(*vertex_positions)[vertex_index * 4 + 1],
            &(*vertex_positions)[vertex_index * 4 + 2]) != 3) {
        fprintf(stderr, "Error parsing obj file '%s' '%s'.\n", fname, line_buffer);
        exit(1);
      }
      (*vertex_positions)[vertex_index * 4 + 3] = 1.0f;
      vertex_index++;
    }
    if (line_buffer[0] == 'f' && line_buffer[1] == ' ') {
      if (sscanf(line_buffer, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d\n",
            &(*vertex_indices)[face_index * 3],
            &(*vertex_indices)[face_index * 3 + 1],
            &(*vertex_indices)[face_index * 3 + 2]) != 3) {
        fprintf(stderr, "Error parsing obj file '%s' '%s'.\n", fname, line_buffer);
        exit(1);
      }
      (*vertex_indices)[face_index * 3]--;
      (*vertex_indices)[face_index * 3 + 1]--;
      (*vertex_indices)[face_index * 3 + 2]--;
      face_index++;
    }
  }
  if (ferror(file)) {
    fprintf(stderr, "Error while reading obj file '%s'.\n", fname);
    exit(1);
  }
  fclose(file);
}
