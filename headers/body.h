#pragma once
#include "mathc.h"
#include "display.h"
#include "trail.h"
#define BODY_DEFAULT_MASS 200
#define BODY_RADIUS_FACTOR 200.f
#define BODY_SPEED_LIMIT 100.f // Pixels per second
#define BODY_SPLIT_VELOCITY 50.f // Speed at which bodies split on impact

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
void body_update(
    Body *body,
    Body *bodies,
    float delta_time,
    sfUint8 sim_speed_multiplier
    );
int body_compare_x_axis(const void *a, const void *b);
sfUint32 body_sweep_and_prune(Body *bodies, Body **possible_collisions);
void body_handle_collision(
    Display *display,
    Body *a,
    Body *b,
    Body *bodies,
    sfUint32 *num_of_bodies
    );
void body_check_collisions(
    Display *display,
    Body **possible_collisions,
    Body *bodies,
    sfUint32 *num_of_bodies
    );
void body_apply_mass(Body *body, sfUint32 mass);
void body_apply_force(Body *body, mfloat_t *force);
void body_calculate_gravitation_force(mfloat_t *result, Body *a, Body *b, float dist2);
void body_get_position(Body *body, mfloat_t *result);
void body_render(Display *display, Body *body);