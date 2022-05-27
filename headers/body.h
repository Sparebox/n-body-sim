#pragma once
#include "mathc.h"
#include "display.h"
#include "trail.h"
#define BODY_DEFAULT_MASS 4000
#define BODY_RADIUS_FACTOR 8000.f
#define BODY_SPEED_LIMIT 100.f // Pixels per second

typedef struct {
    mfloat_t vel[VEC2_SIZE];
    mfloat_t acc[VEC2_SIZE];
    Trail trail;
    sfUint32 mass;
    sfCircleShape *shape;
    sfText *info_text;
} Body;

Body* body_create(Display *display, Body *bodies, sfUint32 *num_of_bodies, float x, float y, sfUint32 mass);
void body_destroy(Body *body, Body *bodies, sfUint32 *num_of_bodies);
void body_destroy_all(Body *bodies, sfUint32 *num_of_bodies);
void body_update(Body *body, float delta_time);
sfUint32 body_sweep_and_prune(Body *bodies, Body **possible_collisions);
void body_check_collisions(Body **possible_collisions, Body *bodies, sfUint32 *num_of_bodies);
void body_apply_mass(Body *body, sfUint32 mass);
void body_apply_force(Body *body, const mfloat_t *force);
void body_get_position(Body *body, mfloat_t *result);
void body_render(Display *display, Body *body);