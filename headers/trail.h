#pragma once
#include "display.h"
#define MAX_TRAIL_VERTICES 128
#define TRAIL_DELAY 100 // Milliseconds

typedef struct {
    sfVertex vertices[MAX_TRAIL_VERTICES];
    sfUint32 current_index;
    sfColor color;
    sfClock *trail_timer;
} Trail;

void trail_append(Trail *trail, float x, float y);
void trail_render(Display *display, Trail *trail);
void trail_reset(Trail *trail);