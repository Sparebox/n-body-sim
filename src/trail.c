#include "trail.h"

void trail_append(Trail *trail, float x, float y)
{
    trail->vertices[trail->current_index].position.x = x;
    trail->vertices[trail->current_index].position.y = y;
    trail->vertices[trail->current_index].color = trail->color;
    trail->current_index++;
    if(trail->current_index == MAX_VERTICES)
    {
        trail->current_index = 0;
    }
}

void trail_render(Display *display, Trail *trail)
{
    sfVertex points[2];
    for(size_t i = 0; i < MAX_VERTICES; i++)
    {
        if(i != trail->current_index - 1)
        {
            points[0] = trail->vertices[i];
            if(i + 1 != MAX_VERTICES)
            {
                points[1] = trail->vertices[i + 1];
            }
            else
            {
                points[1] = points[0];
            }
            sfRenderWindow_drawPrimitives(display->render_window, points, 2, sfLines, NULL);
        }
        
    }
}