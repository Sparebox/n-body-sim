#include "trail.h"

void trail_append(Trail *trail, float x, float y)
{
    if(sfTime_asMilliseconds(sfClock_getElapsedTime(trail->trail_timer)) > TRAIL_DELAY)
    {
        trail->vertices[trail->current_index].position.x = x;
        trail->vertices[trail->current_index].position.y = y;
        trail->vertices[trail->current_index].color = trail->color;
        trail->current_index++;
        if(trail->current_index == MAX_TRAIL_VERTICES)
        {
            trail->current_index = 0;
        }
        sfClock_restart(trail->trail_timer);
    }
}

void trail_render(Display *display, Trail *trail)
{
    sfVertex points[2] = { 0 };
    for(size_t i = 0; i < MAX_TRAIL_VERTICES - 1; i++)
    {
        if(i == trail->current_index - 1)
        {
             points[0] = trail->vertices[MAX_TRAIL_VERTICES - 1];
             points[1] = trail->vertices[0];
        }
        else
        {
            points[0] = trail->vertices[i];
            points[1] = trail->vertices[i + 1];
        }
        const sfVector2i pos0 = 
            sfRenderWindow_mapCoordsToPixel(display->render_window, points[0].position, display->view);
        const sfVector2i pos1 = 
            sfRenderWindow_mapCoordsToPixel(display->render_window, points[1].position, display->view);
        const sfIntRect viewport = {0, 0, WIN_WIDTH, WIN_HEIGHT};
        if(sfIntRect_contains(&viewport, pos0.x, pos0.y) && sfIntRect_contains(&viewport, pos1.x, pos1.y)) // Trail part is in window
        {
            sfRenderWindow_drawPrimitives(display->render_window, points, 2, sfLines, NULL);
        }
    }
}