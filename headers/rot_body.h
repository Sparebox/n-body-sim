#pragma once
#include "display.h"
#include "mathc.h"
#define ROT_BODY_DENSITY 1.5e2f
#define ROTATION_FRICTION_COEFF 1.f

typedef struct {
    mfloat_t vel[VEC2_SIZE];
    mfloat_t acc[VEC2_SIZE];
    mfloat_t last_acc[VEC2_SIZE];
    float angular_vel;
    float angular_acc;
    float last_angular_acc;
    sfUint32 mass;
    float moment_of_inertia;
    sfRectangleShape *shape;
    sfBool is_alive;
    sfText *info_text;
} Rot_body;

void rot_body_update(Rot_body *body, const float delta_time);
void rot_body_render(Display *display, Rot_body *body);
Rot_body rot_body_create(Display *display, const float x, const float y, const float width, const float height);
void rot_body_destroy(Rot_body *body);
sfBool rot_body_is_alive(const Rot_body *body);
void rot_body_apply_torque(Rot_body *body, const float torque);
void rot_body_apply_force(Rot_body *body, mfloat_t *force);