#pragma once
#include "display.h"
#define MAX_VERTICES 128

typedef struct {
    sfVertex vertices[MAX_VERTICES];
    sfUint32 current_index;
    sfColor color;
} Trail;

void trail_append(Trail *trail, float x, float y);
void trail_render(Display *display, Trail *trail);