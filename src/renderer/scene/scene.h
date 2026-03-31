#ifndef XSHARP_SCENE_H
#define XSHARP_SCENE_H

#include "../core/renderer.h"
#include "../core/mesh.h"
#include "../core/texture.h"
#include "../core/pipeline.h"
#include "../shaders/shader.h"

/* ------------------------------------------------------------------ */
/*  Node types                                                         */
/* ------------------------------------------------------------------ */

typedef enum {
    NODE_EMPTY,
    NODE_MESH,
    NODE_CAMERA,
    NODE_LIGHT
} NodeType;

/* ------------------------------------------------------------------ */
/*  Light                                                              */
/* ------------------------------------------------------------------ */

typedef enum {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_SPOT
} LightType;

typedef struct {
    LightType type;
    Vec3      color;
    float     intensity;
    /* Point / spot */
    float     range;
    float     constant_atten;
    float     linear_atten;
    float     quadratic_atten;
    /* Spot */
    float     inner_cone;    /* radians */
    float     outer_cone;    /* radians */
    Vec3      direction;     /* local-space direction (for spot/directional) */
} SceneLight;

/* ------------------------------------------------------------------ */
/*  Camera                                                             */
/* ------------------------------------------------------------------ */

typedef enum {
    CAMERA_PERSPECTIVE,
    CAMERA_ORTHOGRAPHIC
} CameraType;

typedef struct {
    CameraType type;
    float      fov;          /* radians, for perspective */
    float      aspect;
    float      near_plane;
    float      far_plane;
    /* Orthographic */
    float      ortho_size;   /* half-height */
} SceneCamera;

/* ------------------------------------------------------------------ */
/*  Scene node                                                         */
/* ------------------------------------------------------------------ */

#define SCENE_MAX_CHILDREN 64

typedef struct SceneNode {
    char     name[64];
    NodeType type;

    /* Local transform components */
    Vec3     position;
    Quat     rotation;
    Vec3     scale;

    /* Cached matrices */
    Mat4     local_matrix;
    Mat4     world_matrix;
    bool     dirty;  /* needs matrix recalculation */

    /* Type-specific data */
    union {
        struct {
            Mesh            *mesh;
            ShaderProgram   *shader;
            XSharpTexture   *texture;
        } mesh_data;
        SceneCamera camera;
        SceneLight  light;
    } data;

    /* Hierarchy */
    struct SceneNode *parent;
    struct SceneNode *children[SCENE_MAX_CHILDREN];
    int               child_count;
} SceneNode;

/* ------------------------------------------------------------------ */
/*  Scene                                                              */
/* ------------------------------------------------------------------ */

typedef struct {
    SceneNode  *root;
    SceneNode  *active_camera;
    Vec3        ambient_color;

    /* Collected lights during traversal */
    SceneNode  *lights[32];
    int         light_count;
} Scene;

/* ------------------------------------------------------------------ */
/*  Node API                                                           */
/* ------------------------------------------------------------------ */

SceneNode *scene_node_create(const char *name, NodeType type);
void       scene_node_destroy(SceneNode *node);   /* destroys node and all children */
void       scene_node_add_child(SceneNode *parent, SceneNode *child);
void       scene_node_remove_child(SceneNode *parent, SceneNode *child);

/* Set local transform */
void scene_node_set_position(SceneNode *node, Vec3 pos);
void scene_node_set_rotation(SceneNode *node, Quat rot);
void scene_node_set_scale(SceneNode *node, Vec3 scale);
void scene_node_set_euler(SceneNode *node, float pitch, float yaw, float roll);

/* Compute local matrix from position/rotation/scale */
void scene_node_update_local(SceneNode *node);

/* Recursively update world matrices */
void scene_node_update_world(SceneNode *node, const Mat4 *parent_world);

/* Get world-space position */
Vec3 scene_node_world_position(const SceneNode *node);

/* Find a node by name (recursive) */
SceneNode *scene_node_find(SceneNode *root, const char *name);

/* ------------------------------------------------------------------ */
/*  Scene API                                                          */
/* ------------------------------------------------------------------ */

Scene *scene_create(void);
void   scene_destroy(Scene *scene);

/* Update all transforms in the scene */
void scene_update(Scene *scene);

/* Render the entire scene */
void scene_render(Scene *scene, XSharpRenderer *renderer);

/* ------------------------------------------------------------------ */
/*  Camera helpers                                                     */
/* ------------------------------------------------------------------ */

/* Get view matrix from camera node's world transform */
Mat4 camera_get_view_matrix(const SceneNode *camera_node);

/* Get projection matrix from camera data */
Mat4 camera_get_projection_matrix(const SceneCamera *camera);

#endif /* XSHARP_SCENE_H */
