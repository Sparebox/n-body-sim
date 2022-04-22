#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "sim.h"

void sim_poll_events(Sim *sim) 
{
    sfEvent event;
    sfRenderWindow *window = sim->display.render_window;
    Display *display = &sim->display;
    while(sfRenderWindow_pollEvent(window, &event)) 
    {
        switch(event.type) 
        {
            case sfEvtClosed :
                sfRenderWindow_close(window);
                break;
            case sfEvtMouseWheelScrolled :
                display->zoom_level = event.mouseWheelScroll.delta > 0.f ? display->zoom_level + 0.1f : display->zoom_level - 0.1f;
                const sfVector2f new_size = {WIN_WIDTH * display->zoom_level, WIN_HEIGHT * display->zoom_level};
                sfView_setSize(display->view, new_size);
                break;
            case sfEvtMouseButtonPressed :
                if(event.mouseButton.button == sfMouseLeft)
                {
                    display->last_mouse_pos = sfMouse_getPositionRenderWindow(window);
                }
                else if(event.mouseButton.button == sfMouseRight)
                {
                    sim->following_largest_body = !sim->following_largest_body;
                }
                break;
            case sfEvtKeyPressed :
                if(event.key.code == sfKeyEscape)
                {
                    sfRenderWindow_close(window);
                }
                break;
            default :
                break;
        }
    }
}

void sim_init_gui(Sim *sim)
{
    sim->fps_text = sfText_create();
    sfText_setFont(sim->fps_text, sim->display.font);
    sfText_setColor(sim->fps_text, sfWhite);
    sfVector2f scale = {0.5f, 0.5f};
    sfVector2f pos = {0.f, 0.f};
    sfText_setPosition(sim->fps_text, pos);
    sfText_setScale(sim->fps_text, scale);

    sim->bodies_text = sfText_create();
    sfText_setFont(sim->bodies_text, sim->display.font);
    sfText_setColor(sim->bodies_text, sfWhite);
    pos.x = 0.f;
    pos.y = 15.f;
    sfText_setPosition(sim->bodies_text, pos);
    sfText_setScale(sim->bodies_text, scale);

    sim->largest_mass_text = sfText_create();
    sfText_setFont(sim->largest_mass_text, sim->display.font);
    sfText_setColor(sim->largest_mass_text, sfWhite);
    pos.x = 0.f;
    pos.y = 30.f;
    sfText_setPosition(sim->largest_mass_text, pos);
    sfText_setScale(sim->largest_mass_text, scale);
}

void sim_render_gui(Sim *sim)
{
    sfRenderWindow_setView(sim->display.render_window, sim->display.gui_view);
    sfRenderWindow_drawText(sim->display.render_window, sim->fps_text, NULL);
    sfRenderWindow_drawText(sim->display.render_window, sim->bodies_text, NULL);
    sfRenderWindow_drawText(sim->display.render_window, sim->largest_mass_text, NULL);
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
    Body *body = NULL;
    mfloat_t rng_vel[VEC2_SIZE];
    while(current_radians < 2 * M_PI)
    {
        x = center_x + radius * cos(current_radians);
        y = center_y + radius * sin(current_radians);
        body = body_create(&sim->display, sim->bodies, &sim->num_of_bodies, x, y, BODY_DEFAULT_MASS);
        sim_random_vector(rng_vel, BODY_SPEED_LIMIT, BODY_SPEED_LIMIT);
        vec2_assign(body->vel, rng_vel);
        current_radians += offset_radians;
    }
}

void sim_create_grid(Sim *sim, sfUint32 count, float spacing)
{
    float x = spacing;
    float y = spacing;
    for(size_t i = 0; i < count; i++)
    {
        body_create(&sim->display, sim->bodies, &sim->num_of_bodies, x, y, BODY_DEFAULT_MASS);
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
        body_create(&sim->display, sim->bodies, &sim->num_of_bodies, current_pos[0], current_pos[1], BODY_DEFAULT_MASS);
        vec2_add(current_pos, current_pos, offset);
        vec2_subtract(current_line, pos2, current_pos);
        current_length = vec2_length(current_line);
    }
}

void sim_create_random_distribution(Sim *sim, sfUint32 count)
{
    srand((unsigned int)time(NULL));
    mfloat_t pos[] = {sim_random_uint(10, WIN_WIDTH - 10), sim_random_uint(10, WIN_HEIGHT - 10)};
    mfloat_t rng_vel[VEC2_SIZE];
    Body *body = NULL;
    for(size_t i = 0; i < count; i++)
    {
        body = body_create(&sim->display, sim->bodies, &sim->num_of_bodies, pos[0], pos[1], BODY_DEFAULT_MASS);
        sim_random_vector(rng_vel, 0, 1);
        vec2_assign(body->vel, rng_vel);
        pos[0] = sim_random_uint(0, 5000);
        pos[1] = sim_random_uint(0, 5000);
    }
}

sfUint32 sim_random_uint(sfUint32 min, sfUint32 max) 
{
    return rand() % (max - min + 1) + min;
}

float sim_random_float(float min, float max)
{
    float random = ((float) rand() / (float) RAND_MAX) * max;
    return random < min ? min : random;
}

void sim_random_vector(mfloat_t *result, float min_length, float max_length)
{
    vec2_one(result);
    vec2_rotate(result, result, sim_random_float(0.f, 2 * M_PI));
    vec2_multiply_f(result, result, sim_random_uint(min_length, max_length));
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
    sfText_destroy(sim->fps_text);
    sfText_destroy(sim->bodies_text);
    sfText_destroy(sim->largest_mass_text);
    sfClock_destroy(sim->delta_clock);
    sfView_destroy(sim->display.view);
    sfView_destroy(sim->display.gui_view);
    sfRenderWindow_destroy(sim->display.render_window);
    free(sim);
}