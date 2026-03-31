#include "scene.h"
#include <stdlib.h>
#include <string.h>

/* ================================================================== */
/*  Node creation / destruction                                        */
/* ================================================================== */

SceneNode* scene_node_create(const char* name, NodeType type) {
    SceneNode* n = calloc(1, sizeof(SceneNode));
    if (!n)
        return NULL;
    if (name)
        strncpy(n->name, name, sizeof(n->name) - 1);
    n->type = type;
    n->position = vec3(0, 0, 0);
    n->rotation = quat_identity();
    n->scale = vec3(1, 1, 1);
    n->local_matrix = mat4_identity();
    n->world_matrix = mat4_identity();
    n->dirty = true;

    /* Sensible defaults for type-specific data */
    switch (type) {
    case NODE_CAMERA:
        n->data.camera.type = CAMERA_PERSPECTIVE;
        n->data.camera.fov = DEG2RAD(60.0f);
        n->data.camera.aspect = 16.0f / 9.0f;
        n->data.camera.near_plane = 0.1f;
        n->data.camera.far_plane = 1000.0f;
        n->data.camera.ortho_size = 5.0f;
        break;
    case NODE_LIGHT:
        n->data.light.type = LIGHT_DIRECTIONAL;
        n->data.light.color = vec3(1, 1, 1);
        n->data.light.intensity = 1.0f;
        n->data.light.range = 10.0f;
        n->data.light.constant_atten = 1.0f;
        n->data.light.linear_atten = 0.09f;
        n->data.light.quadratic_atten = 0.032f;
        n->data.light.inner_cone = DEG2RAD(12.5f);
        n->data.light.outer_cone = DEG2RAD(17.5f);
        n->data.light.direction = vec3(0, -1, 0);
        break;
    default:
        break;
    }
    return n;
}

void scene_node_destroy(SceneNode* node) {
    if (!node)
        return;
    for (int i = 0; i < node->child_count; i++)
        scene_node_destroy(node->children[i]);
    /* Note: we do NOT free mesh/texture/shader -- those are owned externally */
    free(node);
}

void scene_node_add_child(SceneNode* parent, SceneNode* child) {
    if (!parent || !child)
        return;
    if (parent->child_count >= SCENE_MAX_CHILDREN)
        return;
    child->parent = parent;
    parent->children[parent->child_count++] = child;
    child->dirty = true;
}

void scene_node_remove_child(SceneNode* parent, SceneNode* child) {
    if (!parent || !child)
        return;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            child->parent = NULL;
            /* Shift remaining children */
            for (int j = i; j < parent->child_count - 1; j++)
                parent->children[j] = parent->children[j + 1];
            parent->child_count--;
            return;
        }
    }
}

/* ================================================================== */
/*  Transform                                                          */
/* ================================================================== */

void scene_node_set_position(SceneNode* node, Vec3 pos) {
    if (!node)
        return;
    node->position = pos;
    node->dirty = true;
}

void scene_node_set_rotation(SceneNode* node, Quat rot) {
    if (!node)
        return;
    node->rotation = rot;
    node->dirty = true;
}

void scene_node_set_scale(SceneNode* node, Vec3 s) {
    if (!node)
        return;
    node->scale = s;
    node->dirty = true;
}

void scene_node_set_euler(SceneNode* node, float pitch, float yaw, float roll) {
    if (!node)
        return;
    node->rotation = quat_from_euler(pitch, yaw, roll);
    node->dirty = true;
}

void scene_node_update_local(SceneNode* node) {
    if (!node)
        return;
    Mat4 t = mat4_translate(node->position);
    Mat4 r = quat_to_mat4(node->rotation);
    Mat4 s = mat4_scale(node->scale);
    node->local_matrix = mat4_mul(t, mat4_mul(r, s));
    node->dirty = false;
}

void scene_node_update_world(SceneNode* node, const Mat4* parent_world) {
    if (!node)
        return;

    if (node->dirty)
        scene_node_update_local(node);

    if (parent_world)
        node->world_matrix = mat4_mul(*parent_world, node->local_matrix);
    else
        node->world_matrix = node->local_matrix;

    for (int i = 0; i < node->child_count; i++)
        scene_node_update_world(node->children[i], &node->world_matrix);
}

Vec3 scene_node_world_position(const SceneNode* node) {
    if (!node)
        return vec3(0, 0, 0);
    return vec3(node->world_matrix.m[3][0], node->world_matrix.m[3][1], node->world_matrix.m[3][2]);
}

SceneNode* scene_node_find(SceneNode* root, const char* name) {
    if (!root || !name)
        return NULL;
    if (strcmp(root->name, name) == 0)
        return root;
    for (int i = 0; i < root->child_count; i++) {
        SceneNode* found = scene_node_find(root->children[i], name);
        if (found)
            return found;
    }
    return NULL;
}

/* ================================================================== */
/*  Scene                                                              */
/* ================================================================== */

Scene* scene_create(void) {
    Scene* s = calloc(1, sizeof(Scene));
    if (!s)
        return NULL;
    s->root = scene_node_create("root", NODE_EMPTY);
    s->ambient_color = vec3(0.1f, 0.1f, 0.1f);
    return s;
}

void scene_destroy(Scene* scene) {
    if (!scene)
        return;
    scene_node_destroy(scene->root);
    free(scene);
}

void scene_update(Scene* scene) {
    if (!scene || !scene->root)
        return;
    scene_node_update_world(scene->root, NULL);
}

/* ================================================================== */
/*  Camera                                                             */
/* ================================================================== */

Mat4 camera_get_view_matrix(const SceneNode* cam) {
    if (!cam || cam->type != NODE_CAMERA)
        return mat4_identity();
    /* View matrix is the inverse of the camera's world transform */
    return mat4_inverse(cam->world_matrix);
}

Mat4 camera_get_projection_matrix(const SceneCamera* cam) {
    if (!cam)
        return mat4_identity();
    switch (cam->type) {
    case CAMERA_PERSPECTIVE:
        return mat4_perspective(cam->fov, cam->aspect, cam->near_plane, cam->far_plane);
    case CAMERA_ORTHOGRAPHIC: {
        float h = cam->ortho_size;
        float w = h * cam->aspect;
        return mat4_ortho(-w, w, -h, h, cam->near_plane, cam->far_plane);
    }
    }
    return mat4_identity();
}

/* ================================================================== */
/*  Scene rendering                                                    */
/* ================================================================== */

/* Collect lights from the scene tree */
static void collect_lights(Scene* scene, SceneNode* node) {
    if (!node)
        return;
    if (node->type == NODE_LIGHT && scene->light_count < 32)
        scene->lights[scene->light_count++] = node;
    for (int i = 0; i < node->child_count; i++)
        collect_lights(scene, node->children[i]);
}

/* Render a single node if it has a mesh */
static void render_node(Scene* scene, SceneNode* node, XSharpRenderer* renderer, Mat4 view,
                        Mat4 proj, Vec3 eye_pos, const Frustum* frustum) {
    if (!node)
        return;

    if (node->type == NODE_MESH && node->data.mesh_data.mesh) {
        Mesh* mesh = node->data.mesh_data.mesh;

        /* Frustum culling: test transformed AABB */
        if (frustum) {
            AABB world_bounds = aabb_transform(mesh->bounds, node->world_matrix);
            if (!frustum_test_aabb(frustum, world_bounds))
                goto recurse_children; /* culled */
        }

        PipelineTransforms xforms = pipeline_compute_transforms(node->world_matrix, view, proj);
        xforms.eye_pos = eye_pos;

        PipelineMaterial mat;
        memset(&mat, 0, sizeof(mat));
        mat.shader = node->data.mesh_data.shader;
        mat.diffuse_texture = node->data.mesh_data.texture;
        mat.shadow_bias = 0.005f;

        /* Set up lighting uniforms on the shader */
        if (mat.shader && scene->light_count > 0) {
            /* Use the first directional light for simplicity */
            for (int i = 0; i < scene->light_count; i++) {
                SceneNode* ln = scene->lights[i];
                SceneLight* l = &ln->data.light;
                if (l->type == LIGHT_DIRECTIONAL) {
                    /* Transform light direction to world space */
                    Vec3 world_dir = vec3_normalize(mat4_mul_dir(ln->world_matrix, l->direction));
                    shader_set_vec3(mat.shader, "light_dir", world_dir);
                    shader_set_vec3(mat.shader, "light_color", vec3_mul(l->color, l->intensity));
                    break;
                }
            }
            shader_set_vec3(mat.shader, "ambient", scene->ambient_color);
        }

        pipeline_render_mesh(renderer, mesh, &xforms, &mat);
    }

recurse_children:
    for (int i = 0; i < node->child_count; i++)
        render_node(scene, node->children[i], renderer, view, proj, eye_pos, frustum);
}

void scene_render(Scene* scene, XSharpRenderer* renderer) {
    if (!scene || !renderer || !scene->root)
        return;

    /* Update transforms */
    scene_update(scene);

    /* Get camera matrices */
    Mat4 view = mat4_identity();
    Mat4 proj = mat4_identity();
    Vec3 eye_pos = vec3(0, 0, 0);

    if (scene->active_camera && scene->active_camera->type == NODE_CAMERA) {
        view = camera_get_view_matrix(scene->active_camera);
        proj = camera_get_projection_matrix(&scene->active_camera->data.camera);
        eye_pos = scene_node_world_position(scene->active_camera);
    }

    /* Build frustum for culling */
    Mat4 vp = mat4_mul(proj, view);
    Frustum frustum = frustum_from_vp(vp);

    /* Collect lights */
    scene->light_count = 0;
    collect_lights(scene, scene->root);

    /* Render all nodes */
    render_node(scene, scene->root, renderer, view, proj, eye_pos, &frustum);
}
