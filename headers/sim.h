#pragma once
#include "display.h"
#include "body.h"
#define MAX_BODIES 4096
#define GRAVITATIONAL_CONSTANT 1000
#define VELOCITY_LIMITED 1

typedef struct {
    sfClock *delta_clock;
    sfTime delta_time;
    sfText *fps_text;
    sfText *bodies_text;
    sfText *largest_mass_text;
    sfUint32 num_of_bodies;
    sfBool following_largest_body;
    Display display;
    Body bodies[MAX_BODIES];
    Body *largest_body;
} Sim;

void sim_init_gui(Sim *sim);
void sim_render_gui(Sim *sim);
void sim_destroy(Sim *sim);
void sim_create_circle(
    Sim *sim, 
    const float center_x, 
    const float center_y, 
    float radius, 
    sfUint32 count);
void sim_create_grid(Sim *sim, sfUint32 count, float spacing);
void sim_create_line(Sim *sim, float x1, float y1, float x2, float y2, float spacing);
void sim_create_random_distribution(Sim *sim, sfUint32 count);
sfUint32 sim_random_uint(sfUint32 min, sfUint32 max);
float sim_random_float(float min, float max);
void sim_random_vector(mfloat_t *result, float min_length, float max_length);
void sim_poll_events(Sim *sim);