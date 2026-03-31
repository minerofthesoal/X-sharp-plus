/*
 * X# Standard Library - Physics Module Implementation
 * =====================================================
 * 2D rigid body physics with AABB collision detection,
 * Verlet integration, and impulse-based collision response.
 */

#include "physics_lib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PHYS_MAX_BODIES 1024

typedef struct {
    int     active;
    int     id;
    /* Position and velocity */
    double  x, y;
    double  vx, vy;
    /* Force accumulator */
    double  fx, fy;
    /* Properties */
    double  mass;
    double  inv_mass;  /* 1/mass, 0 for static */
    double  friction;
    double  bounce;    /* restitution */
    /* AABB half-extents */
    double  hw, hh;    /* half-width, half-height */
    int     is_static;
} PhysBody;

typedef struct {
    PhysBody bodies[PHYS_MAX_BODIES];
    int      body_count;
    int      next_id;
    double   gravity_x, gravity_y;
} PhysWorld;

static PhysWorld* get_world(XsValue v) {
    if (v.type != VAL_ENTITY) return NULL;
    XsValue p = xs_entity_get((XsEntity*)v.object, "_phys_ptr");
    if (p.type == VAL_BLADE) return (PhysWorld*)(intptr_t)p.blade;
    return NULL;
}

static XsValue wrap_world(PhysWorld* w) {
    XsEntity* e = xs_entity_new();
    XsValue p; p.type = VAL_BLADE; p.blade = (int64_t)(intptr_t)w;
    xs_entity_set(e, "_phys_ptr", p);
    return xs_entity(e);
}

static PhysBody* find_body(PhysWorld* w, int id) {
    for (int i = 0; i < PHYS_MAX_BODIES; i++) {
        if (w->bodies[i].active && w->bodies[i].id == id) return &w->bodies[i];
    }
    return NULL;
}

/* ===== createWorld(gravityX?, gravityY?) ===== */

XsValue xs_phys_createWorld(int argc, XsValue* args) {
    PhysWorld* w = (PhysWorld*)calloc(1, sizeof(PhysWorld));
    w->gravity_x = (argc >= 1) ? xs_as_spark(args[0]) : 0.0;
    w->gravity_y = (argc >= 2) ? xs_as_spark(args[1]) : 9.81;
    w->next_id = 1;
    return wrap_world(w);
}

/* ===== addBody(world, x, y, width, height, isStatic?) -> blade (id) ===== */

XsValue xs_phys_addBody(int argc, XsValue* args) {
    if (argc < 5) return xs_blade(-1);
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_blade(-1);

    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < PHYS_MAX_BODIES; i++) {
        if (!w->bodies[i].active) { slot = i; break; }
    }
    if (slot < 0) return xs_blade(-1);

    PhysBody* b = &w->bodies[slot];
    memset(b, 0, sizeof(PhysBody));
    b->active = 1;
    b->id = w->next_id++;
    b->x = xs_as_spark(args[1]);
    b->y = xs_as_spark(args[2]);
    b->hw = xs_as_spark(args[3]) / 2.0;
    b->hh = xs_as_spark(args[4]) / 2.0;
    b->is_static = (argc >= 6) ? (int)xs_as_spark(args[5]) : 0;
    b->mass = b->is_static ? 0.0 : 1.0;
    b->inv_mass = b->is_static ? 0.0 : 1.0;
    b->friction = 0.3;
    b->bounce = 0.5;
    w->body_count++;
    return xs_blade((int64_t)b->id);
}

/* ===== removeBody(world, id) ===== */

XsValue xs_phys_removeBody(int argc, XsValue* args) {
    if (argc < 2) return xs_fate(false);
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_fate(false);
    int id = (int)xs_as_spark(args[1]);
    PhysBody* b = find_body(w, id);
    if (!b) return xs_fate(false);
    b->active = 0;
    w->body_count--;
    return xs_fate(true);
}

/* ===== applyForce(world, id, fx, fy) ===== */

XsValue xs_phys_applyForce(int argc, XsValue* args) {
    if (argc < 4) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b || b->is_static) return xs_abyss();
    b->fx += xs_as_spark(args[2]);
    b->fy += xs_as_spark(args[3]);
    return xs_abyss();
}

/* ===== applyImpulse(world, id, ix, iy) ===== */

XsValue xs_phys_applyImpulse(int argc, XsValue* args) {
    if (argc < 4) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b || b->is_static) return xs_abyss();
    b->vx += xs_as_spark(args[2]) * b->inv_mass;
    b->vy += xs_as_spark(args[3]) * b->inv_mass;
    return xs_abyss();
}

/* ===== setGravity(world, gx, gy) ===== */

XsValue xs_phys_setGravity(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    w->gravity_x = xs_as_spark(args[1]);
    w->gravity_y = xs_as_spark(args[2]);
    return xs_abyss();
}

/* AABB overlap check */
static int aabb_overlap(PhysBody* a, PhysBody* b,
                        double* overlapX, double* overlapY) {
    double dx = b->x - a->x;
    double dy = b->y - a->y;
    double ox = (a->hw + b->hw) - fabs(dx);
    double oy = (a->hh + b->hh) - fabs(dy);
    if (ox <= 0 || oy <= 0) return 0;
    *overlapX = (dx > 0) ? ox : -ox;
    *overlapY = (dy > 0) ? oy : -oy;
    return 1;
}

/* ===== stepSimulation(world, dt) ===== */

XsValue xs_phys_stepSimulation(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    double dt = xs_as_spark(args[1]);

    /* Integrate: apply gravity and forces, update velocity and position */
    for (int i = 0; i < PHYS_MAX_BODIES; i++) {
        PhysBody* b = &w->bodies[i];
        if (!b->active || b->is_static) continue;

        /* Apply gravity */
        b->fx += w->gravity_x * b->mass;
        b->fy += w->gravity_y * b->mass;

        /* Semi-implicit Euler integration */
        b->vx += (b->fx * b->inv_mass) * dt;
        b->vy += (b->fy * b->inv_mass) * dt;

        /* Apply friction (simple damping) */
        b->vx *= (1.0 - b->friction * dt);
        b->vy *= (1.0 - b->friction * dt);

        b->x += b->vx * dt;
        b->y += b->vy * dt;

        /* Clear force accumulators */
        b->fx = 0.0;
        b->fy = 0.0;
    }

    /* Collision detection and response */
    for (int i = 0; i < PHYS_MAX_BODIES; i++) {
        if (!w->bodies[i].active) continue;
        for (int j = i + 1; j < PHYS_MAX_BODIES; j++) {
            if (!w->bodies[j].active) continue;
            if (w->bodies[i].is_static && w->bodies[j].is_static) continue;

            PhysBody* a = &w->bodies[i];
            PhysBody* b = &w->bodies[j];
            double ox, oy;
            if (!aabb_overlap(a, b, &ox, &oy)) continue;

            /* Resolve along minimum overlap axis */
            double total_inv = a->inv_mass + b->inv_mass;
            if (total_inv == 0.0) continue;

            if (fabs(ox) < fabs(oy)) {
                /* Separate on X axis */
                a->x -= ox * (a->inv_mass / total_inv);
                b->x += ox * (b->inv_mass / total_inv);
                /* Impulse-based velocity response */
                double e = (a->bounce + b->bounce) * 0.5;
                double rel_vx = b->vx - a->vx;
                double nx = (ox > 0) ? 1.0 : -1.0;
                double j_impulse = -(1.0 + e) * rel_vx * nx / total_inv;
                a->vx -= j_impulse * a->inv_mass * nx;
                b->vx += j_impulse * b->inv_mass * nx;
            } else {
                /* Separate on Y axis */
                a->y -= oy * (a->inv_mass / total_inv);
                b->y += oy * (b->inv_mass / total_inv);
                double e = (a->bounce + b->bounce) * 0.5;
                double rel_vy = b->vy - a->vy;
                double ny = (oy > 0) ? 1.0 : -1.0;
                double j_impulse = -(1.0 + e) * rel_vy * ny / total_inv;
                a->vy -= j_impulse * a->inv_mass * ny;
                b->vy += j_impulse * b->inv_mass * ny;
            }
        }
    }
    return xs_abyss();
}

/* ===== raycast(world, x, y, dx, dy, maxDist) -> entity or abyss ===== */

XsValue xs_phys_raycast(int argc, XsValue* args) {
    if (argc < 6) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    double rx = xs_as_spark(args[1]), ry = xs_as_spark(args[2]);
    double rdx = xs_as_spark(args[3]), rdy = xs_as_spark(args[4]);
    double maxDist = xs_as_spark(args[5]);

    /* Normalize direction */
    double len = sqrt(rdx * rdx + rdy * rdy);
    if (len < 1e-12) return xs_abyss();
    rdx /= len; rdy /= len;

    double closest_t = maxDist;
    int closest_id = -1;

    for (int i = 0; i < PHYS_MAX_BODIES; i++) {
        PhysBody* b = &w->bodies[i];
        if (!b->active) continue;

        /* Ray-AABB intersection using slab method */
        double tmin, tmax;
        double inv_dx = (fabs(rdx) > 1e-12) ? 1.0 / rdx : 1e12;
        double inv_dy = (fabs(rdy) > 1e-12) ? 1.0 / rdy : 1e12;

        double t1 = (b->x - b->hw - rx) * inv_dx;
        double t2 = (b->x + b->hw - rx) * inv_dx;
        if (t1 > t2) { double tmp = t1; t1 = t2; t2 = tmp; }
        tmin = t1; tmax = t2;

        double t3 = (b->y - b->hh - ry) * inv_dy;
        double t4 = (b->y + b->hh - ry) * inv_dy;
        if (t3 > t4) { double tmp = t3; t3 = t4; t4 = tmp; }
        if (t3 > tmin) tmin = t3;
        if (t4 < tmax) tmax = t4;

        if (tmin > tmax || tmax < 0) continue;
        double t = (tmin >= 0) ? tmin : tmax;
        if (t >= 0 && t < closest_t) {
            closest_t = t;
            closest_id = b->id;
        }
    }

    if (closest_id < 0) return xs_abyss();
    XsEntity* result = xs_entity_new();
    xs_entity_set(result, "bodyId", xs_blade(closest_id));
    xs_entity_set(result, "distance", xs_spark(closest_t));
    xs_entity_set(result, "hitX", xs_spark(rx + rdx * closest_t));
    xs_entity_set(result, "hitY", xs_spark(ry + rdy * closest_t));
    return xs_entity(result);
}

/* ===== checkCollision(world, idA, idB) -> fate ===== */

XsValue xs_phys_checkCollision(int argc, XsValue* args) {
    if (argc < 3) return xs_fate(false);
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_fate(false);
    PhysBody* a = find_body(w, (int)xs_as_spark(args[1]));
    PhysBody* b = find_body(w, (int)xs_as_spark(args[2]));
    if (!a || !b) return xs_fate(false);
    double ox, oy;
    return xs_fate(aabb_overlap(a, b, &ox, &oy));
}

/* ===== setMass(world, id, mass) ===== */

XsValue xs_phys_setMass(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b || b->is_static) return xs_abyss();
    b->mass = xs_as_spark(args[2]);
    b->inv_mass = (b->mass > 0) ? 1.0 / b->mass : 0.0;
    return xs_abyss();
}

/* ===== setFriction(world, id, friction) ===== */

XsValue xs_phys_setFriction(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b) return xs_abyss();
    b->friction = xs_as_spark(args[2]);
    return xs_abyss();
}

/* ===== setBounce(world, id, restitution) ===== */

XsValue xs_phys_setBounce(int argc, XsValue* args) {
    if (argc < 3) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b) return xs_abyss();
    b->bounce = xs_as_spark(args[2]);
    return xs_abyss();
}

/* ===== getBodyState(world, id) -> entity with x,y,vx,vy ===== */

XsValue xs_phys_getBodyState(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b) return xs_abyss();
    XsEntity* e = xs_entity_new();
    xs_entity_set(e, "x", xs_spark(b->x));
    xs_entity_set(e, "y", xs_spark(b->y));
    xs_entity_set(e, "vx", xs_spark(b->vx));
    xs_entity_set(e, "vy", xs_spark(b->vy));
    xs_entity_set(e, "mass", xs_spark(b->mass));
    xs_entity_set(e, "width", xs_spark(b->hw * 2));
    xs_entity_set(e, "height", xs_spark(b->hh * 2));
    return xs_entity(e);
}

/* ===== Position / Velocity accessors ===== */

XsValue xs_phys_getPosition(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b) return xs_abyss();
    XsEntity* e = xs_entity_new();
    xs_entity_set(e, "x", xs_spark(b->x));
    xs_entity_set(e, "y", xs_spark(b->y));
    return xs_entity(e);
}

XsValue xs_phys_getVelocity(int argc, XsValue* args) {
    if (argc < 2) return xs_abyss();
    PhysWorld* w = get_world(args[0]);
    if (!w) return xs_abyss();
    PhysBody* b = find_body(w, (int)xs_as_spark(args[1]));
    if (!b) return xs_abyss();
    XsEntity* e = xs_entity_new();
    xs_entity_set(e, "x", xs_spark(b->vx));
    xs_entity_set(e, "y", xs_spark(b->vy));
    return xs_entity(e);
}

/* ===== Registration ===== */

void xs_physics_register(VM* vm) {
    vm_register_native(vm, "Physics.createWorld",     xs_phys_createWorld);
    vm_register_native(vm, "Physics.addBody",         xs_phys_addBody);
    vm_register_native(vm, "Physics.removeBody",      xs_phys_removeBody);
    vm_register_native(vm, "Physics.applyForce",      xs_phys_applyForce);
    vm_register_native(vm, "Physics.applyImpulse",    xs_phys_applyImpulse);
    vm_register_native(vm, "Physics.setGravity",      xs_phys_setGravity);
    vm_register_native(vm, "Physics.stepSimulation",  xs_phys_stepSimulation);
    vm_register_native(vm, "Physics.raycast",         xs_phys_raycast);
    vm_register_native(vm, "Physics.checkCollision",  xs_phys_checkCollision);
    vm_register_native(vm, "Physics.setMass",         xs_phys_setMass);
    vm_register_native(vm, "Physics.setFriction",     xs_phys_setFriction);
    vm_register_native(vm, "Physics.setBounce",       xs_phys_setBounce);
    vm_register_native(vm, "Physics.getBodyState",    xs_phys_getBodyState);
}
