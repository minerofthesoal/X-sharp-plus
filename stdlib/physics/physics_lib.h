/*
 * X# Standard Library - Physics Module
 * ======================================
 * 2D rigid-body physics: Euler integration, AABB broad-phase,
 * SAT narrow-phase collision detection and response.
 * 14 functions.
 */

#ifndef XS_PHYSICS_LIB_H
#define XS_PHYSICS_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_phys_createWorld(int argc, XsValue* args);
XsValue xs_phys_addBody(int argc, XsValue* args);
XsValue xs_phys_removeBody(int argc, XsValue* args);
XsValue xs_phys_applyForce(int argc, XsValue* args);
XsValue xs_phys_applyImpulse(int argc, XsValue* args);
XsValue xs_phys_setGravity(int argc, XsValue* args);
XsValue xs_phys_stepSimulation(int argc, XsValue* args);
XsValue xs_phys_raycast(int argc, XsValue* args);
XsValue xs_phys_checkCollision(int argc, XsValue* args);
XsValue xs_phys_setMass(int argc, XsValue* args);
XsValue xs_phys_setFriction(int argc, XsValue* args);
XsValue xs_phys_setBounce(int argc, XsValue* args);
XsValue xs_phys_getPosition(int argc, XsValue* args);
XsValue xs_phys_getVelocity(int argc, XsValue* args);

void xs_physics_register(VM* vm);

#endif /* XS_PHYSICS_LIB_H */
