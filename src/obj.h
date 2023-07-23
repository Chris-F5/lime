void load_wavefront_obj(struct wavefront_obj *obj, const char *fname);
void destroy_wavefront_obj(struct wavefront_obj *obj);
void wavefront_to_indexed_vertex_obj(struct indexed_vertex_obj *ivo,
    const struct wavefront_obj *wavefront);
void destroy_indexed_vertex_obj(struct indexed_vertex_obj *ivo);
