struct wavefront_obj {
  int position_count, uv_count, normal_count, face_count;
  float *positions, *uvs, *normals;
  int *position_faces, *uv_faces, *normal_faces;
};

struct vertex {
  float pos[3];
  float uv[2];
  float normal[3];
};

struct indexed_vertex_obj {
  int vertex_count, index_count;
  struct vertex *vertices;
  uint32_t *indices;
};
