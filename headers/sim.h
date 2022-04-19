#pragma once
#include "display.h"
#include "body.h"
#define MAX_BODIES 1024

typedef struct {
    sfClock *delta_clock;
    sfTime delta_time;
    Display display;
    Body bodies[MAX_BODIES];
} Sim;

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
void sim_poll_events(Sim *sim);