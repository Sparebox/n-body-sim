#include "trail.h"

void trail_append(Trail *trail, float x, float y)
{
    trail->vertices[trail->current_index].position.x = x;
    trail->vertices[trail->current_index].position.y = y;
    trail->vertices[trail->current_index].color = sfBlue;
    trail->current_index++;
    if(trail->current_index == MAX_VERTICES)
    {
        trail->current_index = 0;
    }
}

void trail_render(Display *display, Trail *trail)
{
    sfRenderWindow_drawPrimitives(display->render_window, trail->vertices, MAX_VERTICES, sfPoints, NULL);
}