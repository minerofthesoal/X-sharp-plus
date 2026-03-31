#ifndef XSHARP_MESH_H
#define XSHARP_MESH_H

#include "math3d.h"
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Vertex format                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    Vec3  position;
    Vec3  normal;
    Vec2  uv;
    Vec4  color;     /* RGBA 0..1 */
} Vertex;

/* ------------------------------------------------------------------ */
/*  Mesh                                                               */
/* ------------------------------------------------------------------ */

typedef struct {
    Vertex  *vertices;
    uint32_t vertex_count;
    uint32_t *indices;
    uint32_t index_count;
    AABB     bounds;
} Mesh;

/* Create from vertex + index data (copies the arrays) */
Mesh *mesh_create(const Vertex *verts, uint32_t vert_count,
                  const uint32_t *indices, uint32_t idx_count);

/* Create an empty mesh with pre-allocated capacity */
Mesh *mesh_create_empty(uint32_t vert_capacity, uint32_t idx_capacity);

void mesh_destroy(Mesh *m);

/* Recalculate bounding box */
void mesh_compute_bounds(Mesh *m);

/* Recalculate smooth normals from triangle faces */
void mesh_compute_normals(Mesh *m);

/* Apply a transformation to all vertices */
void mesh_transform(Mesh *m, Mat4 mat);

/* ------------------------------------------------------------------ */
/*  Primitive generation                                               */
/* ------------------------------------------------------------------ */

Mesh *mesh_create_cube(float size);
Mesh *mesh_create_plane(float width, float depth, int subdiv_x, int subdiv_z);
Mesh *mesh_create_sphere(float radius, int slices, int stacks);
Mesh *mesh_create_cylinder(float radius, float height, int segments);
Mesh *mesh_create_cone(float radius, float height, int segments);
Mesh *mesh_create_torus(float major_r, float minor_r, int major_seg, int minor_seg);

/* ------------------------------------------------------------------ */
/*  OBJ loading (positions, normals, UVs)                              */
/* ------------------------------------------------------------------ */

Mesh *mesh_load_obj(const char *path);

#endif /* XSHARP_MESH_H */
