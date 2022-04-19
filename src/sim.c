#include <stdlib.h>
#include <time.h>
#include "sim.h"

void sim_poll_events(Sim *sim) 
{
    sfEvent event;
    sfRenderWindow *window = sim->display.render_window;
    while(sfRenderWindow_pollEvent(window, &event)) 
    {
        switch(event.type) 
        {
            case sfEvtClosed :
                sfRenderWindow_close(window);
                break;
            default :
                break;
        }
    }
}

void sim_create_circle(
    Sim *sim, 
    const float center_x, 
    const float center_y, 
    float radius, 
    sfUint32 count)
{
    const float offset_radians =  2 * M_PI / count;
    float current_radians = 0;
    float x = 0;
    float y = 0;
    while(current_radians < 2 * M_PI)
    {
        x = center_x + radius * cos(current_radians);
        y = center_y + radius * sin(current_radians);
        body_create(&sim->display, sim->bodies, x, y, BODY_DEFAULT_MASS);
        current_radians += offset_radians;
    }
}

void sim_create_grid(Sim *sim, sfUint32 count, float spacing)
{
    float x = spacing;
    float y = spacing;
    for(size_t i = 0; i < count; i++)
    {
        body_create(&sim->display, sim->bodies, x, y, BODY_DEFAULT_MASS);
        x += spacing;
        if(x > WIN_WIDTH)
        {
            x = spacing;
            y += spacing;
        }
    }
}

void sim_create_line(Sim *sim, float x1, float y1, float x2, float y2, float spacing)
{
    mfloat_t current_length = 0;
    mfloat_t length = 0;
    mfloat_t dir[VEC2_SIZE];
    mfloat_t offset[VEC2_SIZE];
    mfloat_t current_line[VEC2_SIZE];
    mfloat_t pos1[] = {x1, y1};
    mfloat_t pos2[] = {x2, y2};
    mfloat_t current_pos[] = {x1, y1};
    vec2_subtract(dir, pos2, pos1);
    length = vec2_length(dir);
    vec2_normalize(dir, dir);
    vec2_multiply_f(offset, dir, spacing);
    while(current_length < length)
    {
        body_create(&sim->display, sim->bodies, current_pos[0], current_pos[1], BODY_DEFAULT_MASS);
        vec2_add(current_pos, current_pos, offset);
        vec2_subtract(current_line, pos2, current_pos);
        current_length = vec2_length(current_line);
    }
}

void sim_create_random_distribution(Sim *sim, sfUint32 count)
{
    srand((unsigned int)time(NULL));
    mfloat_t pos[] = {sim_random_uint(10, WIN_WIDTH - 10), sim_random_uint(10, WIN_HEIGHT - 10)};
    for(size_t i = 0; i < count; i++)
    {
        body_create(&sim->display, sim->bodies, pos[0], pos[1], BODY_DEFAULT_MASS);
        pos[0] = sim_random_uint(10, WIN_WIDTH - 10);
        pos[1] = sim_random_uint(10, WIN_HEIGHT - 10);
    }
}

sfUint32 sim_random_uint(sfUint32 min, sfUint32 max) 
{
    return rand() % (max - min + 1) + min;
}

void sim_destroy(Sim *sim)
{
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL)
        {
            sfCircleShape_destroy(sim->bodies[i].shape);
            sfText_destroy(sim->bodies[i].info_text);
        }
    }
    sfClock_destroy(sim->delta_clock);
    sfView_destroy(sim->display.view);
    sfRenderWindow_destroy(sim->display.render_window);
    free(sim);
}