#include "pipeline.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ================================================================== */
/*  Transform computation                                              */
/* ================================================================== */

PipelineTransforms pipeline_compute_transforms(Mat4 model, Mat4 view, Mat4 projection) {
    PipelineTransforms t;
    t.model      = model;
    t.view       = view;
    t.projection = projection;
    t.mvp        = mat4_mul(projection, mat4_mul(view, model));
    t.normal_mat = mat4_transpose(mat4_inverse(model));
    /* Extract eye position from inverse view matrix */
    Mat4 inv_view = mat4_inverse(view);
    t.eye_pos = vec3(inv_view.m[3][0], inv_view.m[3][1], inv_view.m[3][2]);
    return t;
}

/* ================================================================== */
/*  Clipping (Sutherland-Hodgman against all 6 frustum planes)         */
/* ================================================================== */

/* A vertex in clip space with all attributes */
typedef struct {
    Vec4           clip_pos;
    VertexInput    vin;     /* original vertex data for shader input */
    ShaderVaryings vary;
} ClipVertex;

/* Lerp between two ClipVertices */
static ClipVertex clip_lerp(const ClipVertex *a, const ClipVertex *b, float t) {
    ClipVertex r;
    r.clip_pos = vec4_lerp(a->clip_pos, b->clip_pos, t);
    r.vin.position = vec3_lerp(a->vin.position, b->vin.position, t);
    r.vin.normal   = vec3_lerp(a->vin.normal, b->vin.normal, t);
    r.vin.uv       = vec2_lerp(a->vin.uv, b->vin.uv, t);
    r.vin.color    = vec4_lerp(a->vin.color, b->vin.color, t);
    for (int i = 0; i < SHADER_MAX_VARYINGS; i++)
        r.vary.v[i] = lerpf(a->vary.v[i], b->vary.v[i], t);
    return r;
}

/*
 * Clip against one plane.  The plane is defined by which component (0=x,1=y,2=z)
 * and sign (+1 or -1).  A clip-space point is inside if:
 *   sign * component <= w
 * i.e.  sign*component - w <= 0
 */
static int clip_against_plane(const ClipVertex *in, int in_count,
                               ClipVertex *out, int axis, float sign) {
    if (in_count == 0) return 0;
    int out_count = 0;

    for (int i = 0; i < in_count; i++) {
        const ClipVertex *cur = &in[i];
        const ClipVertex *prev = &in[(i + in_count - 1) % in_count];

        /* Distance to clip plane: d = sign*component - w   (inside when d <= 0) */
        float dc, dp;
        float *cc = (float*)&cur->clip_pos;
        float *pc = (float*)&prev->clip_pos;
        dc = sign * cc[axis] - cc[3];  /* w is index 3 */
        dp = sign * pc[axis] - pc[3];

        bool cur_inside  = (dc <= 0);
        bool prev_inside = (dp <= 0);

        if (prev_inside != cur_inside) {
            /* Edge crosses plane - add intersection */
            float t = dp / (dp - dc);
            if (out_count < 12) out[out_count++] = clip_lerp(prev, cur, t);
        }
        if (cur_inside) {
            if (out_count < 12) out[out_count++] = *cur;
        }
    }
    return out_count;
}

/* Full Sutherland-Hodgman clip against all 6 planes of the canonical view volume:
   -w <= x <= w,  -w <= y <= w,  -w <= z <= w  (OpenGL-style NDC before divide) */
static int clip_triangle(const ClipVertex tri[3], ClipVertex *out) {
    ClipVertex buf_a[12], buf_b[12];
    memcpy(buf_a, tri, sizeof(ClipVertex) * 3);
    int count = 3;

    /* +x: x <= w  =>  +1*x - w <= 0 */
    count = clip_against_plane(buf_a, count, buf_b, 0,  1.0f); if (count < 3) return 0;
    /* -x: -x <= w => -1*x - w <= 0 */
    count = clip_against_plane(buf_b, count, buf_a, 0, -1.0f); if (count < 3) return 0;
    /* +y */
    count = clip_against_plane(buf_a, count, buf_b, 1,  1.0f); if (count < 3) return 0;
    /* -y */
    count = clip_against_plane(buf_b, count, buf_a, 1, -1.0f); if (count < 3) return 0;
    /* +z (far) */
    count = clip_against_plane(buf_a, count, buf_b, 2,  1.0f); if (count < 3) return 0;
    /* -z (near): note for OpenGL NDC, near plane is -w <= z => -1*z - w <= 0
       But we use 0 <= z <= w for depth, so near is:  0 <= z  => -z <= 0 ...
       Actually for standard clip: -w <= z <= w, so we clip -z - w <= 0 */
    count = clip_against_plane(buf_b, count, buf_a, 2, -1.0f); if (count < 3) return 0;

    memcpy(out, buf_a, sizeof(ClipVertex) * count);
    return count;
}

/* ================================================================== */
/*  Viewport transform                                                 */
/* ================================================================== */

static RasterVertex to_screen(const ClipVertex *cv, const Rect *vp, int varying_count) {
    RasterVertex rv;
    memset(&rv, 0, sizeof(rv));

    /* Perspective divide */
    float inv_w = 1.0f / cv->clip_pos.w;
    float ndx = cv->clip_pos.x * inv_w;
    float ndy = cv->clip_pos.y * inv_w;
    float ndz = cv->clip_pos.z * inv_w;

    /* NDC [-1,1] -> viewport */
    rv.position.x = (ndx * 0.5f + 0.5f) * vp->width + vp->x;
    rv.position.y = (1.0f - (ndy * 0.5f + 0.5f)) * vp->height + vp->y; /* flip Y */
    rv.position.z = ndz * 0.5f + 0.5f;  /* depth [0,1] */
    rv.position.w = inv_w;               /* store 1/w for perspective-correct interp */

    /* Copy interpolated attributes from varyings */
    rv.world_pos = vec3(cv->vary.v[0], cv->vary.v[1], cv->vary.v[2]);
    rv.normal    = vec3(cv->vary.v[3], cv->vary.v[4], cv->vary.v[5]);
    rv.uv        = vec2(cv->vary.v[6], cv->vary.v[7]);
    rv.color     = vec4(cv->vary.v[8], cv->vary.v[9], cv->vary.v[10], cv->vary.v[11]);

    (void)varying_count;
    return rv;
}

/* ================================================================== */
/*  Shadow test                                                        */
/* ================================================================== */

static float shadow_test(const PipelineMaterial *mat, Vec3 world_pos) {
    if (!mat->shadow_enabled || !mat->shadow_map) return 1.0f;

    Vec4 light_clip = mat4_mul_vec4(mat->light_vp, vec4_from_vec3(world_pos, 1.0f));
    if (light_clip.w <= 0.0f) return 1.0f;

    float inv_w = 1.0f / light_clip.w;
    float sx = light_clip.x * inv_w * 0.5f + 0.5f;
    float sy = light_clip.y * inv_w * 0.5f + 0.5f;
    float sz = light_clip.z * inv_w * 0.5f + 0.5f;

    int px = (int)(sx * mat->shadow_map->width);
    int py = (int)(sy * mat->shadow_map->height);

    if (px < 0 || px >= mat->shadow_map->width || py < 0 || py >= mat->shadow_map->height)
        return 1.0f;

    float stored_depth = mat->shadow_map->depth[py * mat->shadow_map->width + px];
    return (sz - mat->shadow_bias > stored_depth) ? 0.3f : 1.0f;
}

/* ================================================================== */
/*  Pipeline: render a single triangle                                 */
/* ================================================================== */

/*
 * Full per-fragment rendering with programmable shaders.
 * Similar to renderer_draw_triangle but invokes the fragment shader.
 */
static void rasterize_with_shader(XSharpRenderer *r,
                                   const ClipVertex *cv, int vert_count,
                                   const PipelineMaterial *mat,
                                   const Rect *vp) {
    ShaderProgram *shader = mat->shader;
    Framebuffer *fb = r->framebuffer;
    const RenderState *st = &r->state;
    int varying_count = shader ? shader->varying_count : 12;

    /* Convert clipped polygon to screen-space, then fan-triangulate */
    RasterVertex screen[12];
    for (int i = 0; i < vert_count; i++)
        screen[i] = to_screen(&cv[i], vp, varying_count);

    for (int t = 2; t < vert_count; t++) {
        const RasterVertex *tri[3] = { &screen[0], &screen[t-1], &screen[t] };

        /* Backface culling in screen space */
        if (st->backface_cull) {
            float ax = tri[1]->position.x - tri[0]->position.x;
            float ay = tri[1]->position.y - tri[0]->position.y;
            float bx = tri[2]->position.x - tri[0]->position.x;
            float by = tri[2]->position.y - tri[0]->position.y;
            if (ax * by - ay * bx <= 0.0f) continue;
        }

        /* Wireframe mode */
        if (st->wireframe) {
            Color c = color_from_vec4(tri[0]->color);
            renderer_draw_line(r, (int)tri[0]->position.x, (int)tri[0]->position.y,
                                  (int)tri[1]->position.x, (int)tri[1]->position.y, c);
            renderer_draw_line(r, (int)tri[1]->position.x, (int)tri[1]->position.y,
                                  (int)tri[2]->position.x, (int)tri[2]->position.y, c);
            renderer_draw_line(r, (int)tri[2]->position.x, (int)tri[2]->position.y,
                                  (int)tri[0]->position.x, (int)tri[0]->position.y, c);
            continue;
        }

        /* Bounding box */
        float fminx = fminf(fminf(tri[0]->position.x, tri[1]->position.x), tri[2]->position.x);
        float fmaxx = fmaxf(fmaxf(tri[0]->position.x, tri[1]->position.x), tri[2]->position.x);
        float fminy = fminf(fminf(tri[0]->position.y, tri[1]->position.y), tri[2]->position.y);
        float fmaxy = fmaxf(fmaxf(tri[0]->position.y, tri[1]->position.y), tri[2]->position.y);

        int minx = maxi((int)floorf(fminx), maxi(0, vp->x));
        int maxx = mini((int)ceilf(fmaxx), mini(fb->width - 1, vp->x + vp->width - 1));
        int miny = maxi((int)floorf(fminy), maxi(0, vp->y));
        int maxy = mini((int)ceilf(fmaxy), mini(fb->height - 1, vp->y + vp->height - 1));
        if (minx > maxx || miny > maxy) continue;

        /* Barycentric setup */
        float x0 = tri[0]->position.x, y0 = tri[0]->position.y;
        float x1 = tri[1]->position.x, y1 = tri[1]->position.y;
        float x2 = tri[2]->position.x, y2 = tri[2]->position.y;
        float denom = (y1-y2)*(x0-x2) + (x2-x1)*(y0-y2);
        if (fabsf(denom) < XSHARP_EPSILON) continue;
        float inv_denom = 1.0f / denom;

        float w0_inv = tri[0]->position.w;
        float w1_inv = tri[1]->position.w;
        float w2_inv = tri[2]->position.w;

        /* Corresponding ClipVertex varyings for this sub-triangle */
        const ShaderVaryings *sv0 = &cv[0].vary;
        const ShaderVaryings *sv1 = &cv[t-1].vary;
        const ShaderVaryings *sv2 = &cv[t].vary;

        for (int py = miny; py <= maxy; py++) {
            for (int px = minx; px <= maxx; px++) {
                float pxf = px + 0.5f, pyf = py + 0.5f;
                float l0 = ((y1-y2)*(pxf-x2) + (x2-x1)*(pyf-y2)) * inv_denom;
                float l1 = ((y2-y0)*(pxf-x2) + (x0-x2)*(pyf-y2)) * inv_denom;
                float l2 = 1.0f - l0 - l1;
                if (l0 < 0 || l1 < 0 || l2 < 0) continue;

                /* Depth */
                float depth = l0*tri[0]->position.z + l1*tri[1]->position.z + l2*tri[2]->position.z;
                if (depth < 0.0f || depth > 1.0f) continue;

                /* Depth test */
                int fi = py * fb->width + px;
                if (st->depth_test && depth >= fb->depth[fi]) continue;

                /* Scissor test */
                if (st->scissor_enable) {
                    if (px < st->scissor.x || px >= st->scissor.x + st->scissor.width ||
                        py < st->scissor.y || py >= st->scissor.y + st->scissor.height)
                        continue;
                }

                /* Perspective-correct weights */
                float inv_w = l0*w0_inv + l1*w1_inv + l2*w2_inv;
                if (fabsf(inv_w) < XSHARP_EPSILON) continue;
                float w = 1.0f / inv_w;
                float pc0 = l0*w0_inv*w, pc1 = l1*w1_inv*w, pc2 = l2*w2_inv*w;

                /* Interpolate varyings */
                ShaderVaryings frag_vary;
                for (int vi = 0; vi < varying_count && vi < SHADER_MAX_VARYINGS; vi++)
                    frag_vary.v[vi] = pc0*sv0->v[vi] + pc1*sv1->v[vi] + pc2*sv2->v[vi];

                /* Run fragment shader */
                Vec4 frag_color;
                if (shader && shader->fragment_fn) {
                    FragmentInput fin;
                    fin.varyings   = frag_vary;
                    fin.screen_pos = vec2((float)px, (float)py);
                    fin.depth      = depth;
                    frag_color = shader->fragment_fn(&fin, shader->uniforms, shader->uniform_count);
                } else {
                    /* Default: vertex color */
                    frag_color = vec4(frag_vary.v[8], frag_vary.v[9], frag_vary.v[10], frag_vary.v[11]);
                }

                /* Shadow test */
                if (mat->shadow_enabled) {
                    Vec3 wp = vec3(frag_vary.v[0], frag_vary.v[1], frag_vary.v[2]);
                    float shadow = shadow_test(mat, wp);
                    frag_color.x *= shadow;
                    frag_color.y *= shadow;
                    frag_color.z *= shadow;
                }

                /* Output */
                Color c = color_from_vec4(frag_color);
                if (st->blend_mode != BLEND_NONE) {
                    Color dst = color_unpack(fb->color[fi]);
                    c = blend_colors(dst, c, st->blend_mode);
                }
                fb->color[fi] = color_pack(c);
                if (st->depth_write)
                    fb->depth[fi] = depth;
            }
        }
    }
}

void pipeline_render_triangle(XSharpRenderer *renderer,
                              const Vertex *v0, const Vertex *v1, const Vertex *v2,
                              const PipelineTransforms *xforms,
                              const PipelineMaterial *material) {
    if (!renderer || !xforms) return;

    ShaderProgram *shader = material ? material->shader : NULL;

    /* Set up shader uniforms from pipeline transforms */
    if (shader) {
        shader_set_mat4(shader, "model", xforms->model);
        shader_set_mat4(shader, "view", xforms->view);
        shader_set_mat4(shader, "projection", xforms->projection);
        shader_set_mat4(shader, "mvp", xforms->mvp);
        shader_set_mat4(shader, "normal_mat", xforms->normal_mat);
        shader_set_vec3(shader, "eye_pos", xforms->eye_pos);
        if (material && material->diffuse_texture)
            shader_set_texture(shader, "diffuse_tex", material->diffuse_texture);
    }

    /* Run vertex shader on each vertex */
    const Vertex *verts[3] = {v0, v1, v2};
    ClipVertex clip_tri[3];

    for (int i = 0; i < 3; i++) {
        VertexInput vin;
        vin.position = verts[i]->position;
        vin.normal   = verts[i]->normal;
        vin.uv       = verts[i]->uv;
        vin.color    = verts[i]->color;

        if (shader && shader->vertex_fn) {
            VertexOutput vout = shader->vertex_fn(&vin, shader->uniforms, shader->uniform_count);
            clip_tri[i].clip_pos = vout.position;
            clip_tri[i].vary     = vout.varyings;
        } else {
            /* Default vertex shader: just MVP transform */
            clip_tri[i].clip_pos = mat4_mul_vec4(xforms->mvp, vec4_from_vec3(vin.position, 1.0f));
            Vec3 wp = mat4_mul_point(xforms->model, vin.position);
            Vec3 wn = vec3_normalize(mat4_mul_dir(xforms->normal_mat, vin.normal));
            clip_tri[i].vary.v[0] = wp.x; clip_tri[i].vary.v[1] = wp.y; clip_tri[i].vary.v[2] = wp.z;
            clip_tri[i].vary.v[3] = wn.x; clip_tri[i].vary.v[4] = wn.y; clip_tri[i].vary.v[5] = wn.z;
            clip_tri[i].vary.v[6] = vin.uv.x; clip_tri[i].vary.v[7] = vin.uv.y;
            clip_tri[i].vary.v[8] = vin.color.x; clip_tri[i].vary.v[9] = vin.color.y;
            clip_tri[i].vary.v[10] = vin.color.z; clip_tri[i].vary.v[11] = vin.color.w;
        }
        clip_tri[i].vin.position = verts[i]->position;
        clip_tri[i].vin.normal   = verts[i]->normal;
        clip_tri[i].vin.uv       = verts[i]->uv;
        clip_tri[i].vin.color    = verts[i]->color;
    }

    /* Clip against view frustum */
    ClipVertex clipped[12];
    int clipped_count = clip_triangle(clip_tri, clipped);
    if (clipped_count < 3) return;

    /* Rasterize with fragment shader */
    rasterize_with_shader(renderer, clipped, clipped_count, material, &renderer->state.viewport);
}

/* ================================================================== */
/*  Pipeline: render a mesh                                            */
/* ================================================================== */

void pipeline_render_mesh(XSharpRenderer *renderer,
                          const Mesh *mesh,
                          const PipelineTransforms *xforms,
                          const PipelineMaterial *material) {
    if (!renderer || !mesh || !xforms) return;
    if (!mesh->vertices || !mesh->indices) return;

    PipelineMaterial default_mat;
    if (!material) {
        memset(&default_mat, 0, sizeof(default_mat));
        default_mat.shadow_bias = 0.005f;
        material = &default_mat;
    }

    /* Process each triangle */
    for (uint32_t i = 0; i + 2 < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i];
        uint32_t i1 = mesh->indices[i+1];
        uint32_t i2 = mesh->indices[i+2];

        if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count)
            continue;

        pipeline_render_triangle(renderer,
                                 &mesh->vertices[i0],
                                 &mesh->vertices[i1],
                                 &mesh->vertices[i2],
                                 xforms, material);
    }
}
