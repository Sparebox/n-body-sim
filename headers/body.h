#pragma once
#include "mathc.h"
#include "display.h"
#include "trail.h"
#define BODY_DEFAULT_MASS 10
#define BODY_RADIUS_FACTOR 50.f
#define BODY_SPEED_LIMIT 300 // Pixels per second

typedef struct {
    mfloat_t vel[VEC2_SIZE];
    mfloat_t acc[VEC2_SIZE];
    Trail trail;
    sfUint32 mass;
    sfUint32 id;
    sfCircleShape *shape;
    sfText *info_text;
} Body;

Body* body_create(Display *display, Body *bodies, sfUint32 *num_of_bodies, float x, float y, sfUint32 mass);
void body_destroy(Body *body, Body *bodies, sfUint32 *num_of_bodies);
void body_update(Display *display, Body *body, Body *bodies, sfUint32 *num_of_bodies, float delta_time);
void body_handle_collision(Body *a, Body *b, Body *bodies, sfUint32 *num_of_bodies);
void body_apply_mass(Body *body, sfUint32 mass);
void body_apply_force(Body *body, mfloat_t *force);
float body_calculate_gravitation_force(Body *a, Body *b);
void body_get_position(Body *body, mfloat_t *result);
void body_render(Display *display, Body *body);