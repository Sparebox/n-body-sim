#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "sim.h"
#include "editor.h"

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
                sim_handle_mouse_scroll(sim, &event);
                break;
            case sfEvtMouseButtonPressed :
                display->last_mouse_click_pos = sfMouse_getPositionRenderWindow(window);
                if(event.mouseButton.button == sfMouseLeft)
                {
                    sim_handle_left_click(sim);
                }
                else if(event.mouseButton.button == sfMouseRight)
                {
                    if(sim->editor_enabled)
                    {

                    }
                    else
                    {
                        sim->following_largest_body = !sim->following_largest_body;
                    }
                }
                break;
            case sfEvtMouseButtonReleased :
                if(event.mouseButton.button == sfMouseLeft)
                {
                    if(sim->editor_enabled)
                    {
                        if(sim->editor.selected_body != NULL)
                        {
                            editor_apply_velocity(&sim->editor, display);
                        }
                        if(sim->editor.circle_mode_enabled)
                        {
                            sim_create_circle(
                                sim,
                                display->last_mouse_click_pos.x,
                                display->last_mouse_click_pos.y,
                                sfCircleShape_getRadius(sim->editor.tool_circle),
                                20,
                                BODY_DEFAULT_MASS,
                                sfTrue);
                        }
                    }
                }
                break;
            case sfEvtKeyPressed :
                switch(event.key.code)
                {
                    case sfKeyEscape :
                        sfRenderWindow_close(window);
                        break;
                    case sfKeySpace :
                        sim->paused = !sim->paused;
                        break;
                    case sfKeyE :
                        sim->editor_enabled = !sim->editor_enabled;
                        if(sim->editor.tool_circle == NULL) // Create tool circle shape once
                        {
                            const sfVector2i mouse_pos = sfMouse_getPositionRenderWindow(display->render_window);
                            sim->editor.tool_circle = sfCircleShape_create();
                            sfCircleShape_setOutlineThickness(sim->editor.tool_circle, 1.f);
                            sfCircleShape_setOutlineColor(sim->editor.tool_circle, sfWhite);
                            sfCircleShape_setFillColor(sim->editor.tool_circle, sfTransparent);
                            sfCircleShape_setPosition(
                                sim->editor.tool_circle,
                                sfRenderWindow_mapPixelToCoords(display->render_window, mouse_pos, display->view)
                            );
                        }
                        break;
                    case sfKeyR :
                        if(sim->editor_enabled)
                        {
                            sim_create_random_distribution(sim, 1000, sfTrue);
                        }
                        break;
                    case sfKeyX :
                        if(sim->editor_enabled)
                        {
                            body_destroy_all(sim->bodies, &sim->num_of_bodies);
                        }   
                        break;
                    case sfKeyC :
                        if(sim->editor_enabled)
                        {
                            sim->editor.circle_mode_enabled = !sim->editor.circle_mode_enabled;
                            const sfVector2i mouse_pos = sfMouse_getPositionRenderWindow(display->render_window);
                            sfCircleShape_setPosition(
                                sim->editor.tool_circle,
                                sfRenderWindow_mapPixelToCoords(display->render_window, mouse_pos, display->view)
                            );
                        }
                        break;
                    default :
                        break;
                }
                break;
            default :
                break;
        }
    }
}

void sim_handle_mouse_scroll(Sim *sim, sfEvent *event)
{
    if(sim->editor_enabled)
    {
        sfFloatRect bounds = { 0 };
        const sfVector2i screen_mouse_pos = sfMouse_getPositionRenderWindow(sim->display.render_window);
        const sfVector2f world_mouse_pos = 
            sfRenderWindow_mapPixelToCoords(sim->display.render_window, screen_mouse_pos, sim->display.view);
        // When cursor is on a body
        for(size_t i = 0; i < MAX_BODIES; i++)
        {
            if(sim->bodies[i].shape == NULL)
            {
                continue;
            }
            bounds = sfCircleShape_getGlobalBounds(sim->bodies[i].shape);
            if(sfFloatRect_contains(&bounds, world_mouse_pos.x, world_mouse_pos.y))
            {
                if(event->mouseWheelScroll.delta > 0.f)
                {
                    body_apply_mass(&sim->bodies[i], sim->bodies[i].mass + BODY_DEFAULT_MASS);
                }
                else if(sim->bodies[i].mass > BODY_DEFAULT_MASS * 2)
                {
                    body_apply_mass(&sim->bodies[i], sim->bodies[i].mass - BODY_DEFAULT_MASS);
                }
            }
        }
        if(event->mouseWheelScroll.delta > 0.f)
        {
            sim->editor.new_body_mass += BODY_DEFAULT_MASS * sim->display.zoom_level;
        }
        else if(sim->editor.new_body_mass > BODY_DEFAULT_MASS * 2 )
        {
            sim->editor.new_body_mass -= BODY_DEFAULT_MASS * sim->display.zoom_level;
        }
    }
    else
    {
        sim->display.zoom_level = 
            event->mouseWheelScroll.delta > 0.f ? 1.1f * sim->display.zoom_level + 0.1f : 0.9f * sim->display.zoom_level - 0.1f;
        sim->display.zoom_level = sim->display.zoom_level < 0.1f ? 0.1f : sim->display.zoom_level;
        const sfVector2f new_size = {WIN_WIDTH * sim->display.zoom_level, WIN_HEIGHT * sim->display.zoom_level};
        sfView_setSize(sim->display.view, new_size);
    }
}

void sim_handle_left_click(Sim *sim)
{
    const sfVector2f world_mouse_pos = 
    sfRenderWindow_mapPixelToCoords(sim->display.render_window, sim->display.last_mouse_click_pos, sim->display.view);
    sfFloatRect bounds = { 0 };
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape == NULL)
        {
            continue;
        }
        bounds = sfCircleShape_getGlobalBounds(sim->bodies[i].shape);
        if(sfFloatRect_contains(&bounds, world_mouse_pos.x, world_mouse_pos.y)) // If mouse is on a body
        {
            if(sim->editor_enabled)
            {
                sim->editor.selected_body = &sim->bodies[i];
                sim->editor.selected_body_pos = sfCircleShape_getPosition(sim->bodies[i].shape);
            }
            else
            {
                sim->followed_body = &sim->bodies[i];
                sim->following_selected_body = sfTrue;
            }
            sim->display.mouse_was_on_body = sfTrue;
            break;
        }
        else
        {
            sim->following_selected_body = sfFalse;
            sim->following_largest_body = sfFalse;
            sim->display.mouse_was_on_body = sfFalse;
        }
    }
    if(!sim->display.mouse_was_on_body && sim->editor_enabled)
    {
        if(sim->editor.circle_mode_enabled)
        {
            sfCircleShape_setPosition(sim->editor.tool_circle, world_mouse_pos);
        }
        else
        {
            body_create(
                &sim->display,
                sim->bodies,
                &sim->num_of_bodies,
                world_mouse_pos.x,
                world_mouse_pos.y,
                sim->editor.new_body_mass
            );
        }
    }
    
}

void sim_init_gui(Sim *sim)
{
    sfVector2f scale = {0.5f, 0.5f};
    sfVector2f pos = {0.f, 0.f};
    Display *const display = &sim->display;
    sim->fps_text = sim_create_text(pos, scale, display->font);
    pos.y += 15.f;
    sim->bodies_text = sim_create_text(pos, scale, display->font);
    pos.y += 15.f;
    sim->largest_mass_text = sim_create_text(pos, scale, display->font);
    pos.y += 15.f;
    sim->zoom_text = sim_create_text(pos, scale, display->font);
    pos.y += 15.f;
    sim->possible_collisions_text = sim_create_text(pos, scale, display->font);
    pos.y += 15.f;
    sim->paused_text = sim_create_text(pos, scale, display->font);
}

void sim_update_gui(Sim *sim)
{
    char string[32];
    sprintf(string, "FPS %.1f", 1.f / sfTime_asSeconds(sim->delta_time));
    sfText_setString(sim->fps_text, string);

    sprintf(string, "BODIES %d / %d", sim->num_of_bodies, MAX_BODIES);
    sfText_setString(sim->bodies_text, string);

    if(sim->largest_body != NULL)
    {
        sprintf(string, "LARGEST MASS %d", sim->largest_body->mass);
    }
    else
    {
        sprintf(string, "LARGEST MASS ");
    }
    sfText_setString(sim->largest_mass_text, string);
    
    sprintf(string, "ZOOMOUT %.1f", sim->display.zoom_level);
    sfText_setString(sim->zoom_text, string);

    sprintf(string, "POSSIBLE COLLISIONS %d", sim->possible_collisions);
    sfText_setString(sim->possible_collisions_text, string);

    sprintf(string, sim->paused ? "PAUSED" : sim->editor_enabled ? "EDITOR" : "RUNNING");
    sfText_setString(sim->paused_text, string);
}

void sim_render_gui(Sim *sim)
{
    sfRenderWindow_setView(sim->display.render_window, sim->display.gui_view);
    sfRenderWindow_drawText(sim->display.render_window, sim->fps_text, NULL);
    sfRenderWindow_drawText(sim->display.render_window, sim->bodies_text, NULL);
    sfRenderWindow_drawText(sim->display.render_window, sim->largest_mass_text, NULL);
    sfRenderWindow_drawText(sim->display.render_window, sim->zoom_text, NULL);
    sfRenderWindow_drawText(sim->display.render_window, sim->possible_collisions_text, NULL);
    sfRenderWindow_drawText(sim->display.render_window, sim->paused_text, NULL);
}

void sim_update(Sim *sim)
{
    sfUint32 largest_mass = 0;
    if(sim->largest_body != NULL)
    {
        largest_mass = sim->largest_body->mass;
    }
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL && !sim->paused)
        {
            if(sim->bodies[i].mass > largest_mass)
            {
                sim->largest_body = &sim->bodies[i];
                largest_mass = sim->largest_body->mass;
            }
            body_update(
                &sim->bodies[i],
                sim->bodies,
                sfTime_asSeconds(sim->delta_time)
            );
        }
    }
    Body *possible_collisions[MAX_BODIES << 1] = { 0 };
    sim->possible_collisions = body_sweep_and_prune(sim->bodies, possible_collisions);
    body_check_collisions(&sim->display, possible_collisions, sim->bodies, &sim->num_of_bodies);
}

void sim_render(Sim *sim)
{
    // Move view
    if(sim->following_largest_body && sim->largest_body->shape != NULL)
    {
        const sfVector2f largest_body_pos = sfCircleShape_getPosition(sim->largest_body->shape);
        sfView_setCenter(sim->display.view, largest_body_pos);
    }
    else if(sim->following_selected_body && sim->followed_body->shape != NULL)
    {
        const sfVector2f followed_body_pos = sfCircleShape_getPosition(sim->followed_body->shape);
        sfView_setCenter(sim->display.view, followed_body_pos);
    }
    sfRenderWindow_setView(sim->display.render_window, sim->display.view);
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL)
        {
            body_render(&sim->display, &sim->bodies[i]);
        }
    }
    if(sim->editor_enabled)
    {
        editor_render(sim);
    }
}

sfText* sim_create_text(sfVector2f pos, sfVector2f scale, sfFont *font)
{
    sfText *text = sfText_create();
    sfText_setFont(text, font);
    sfText_setColor(text, sfWhite);
    sfText_setPosition(text, pos);
    sfText_setScale(text, scale);
    return text;
}

void sim_create_circle(
    Sim *sim, 
    const float center_x, 
    const float center_y, 
    float radius, 
    sfUint32 count,
    sfUint32 mass,
    sfBool stationary)
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
        body = body_create(&sim->display, sim->bodies, &sim->num_of_bodies, x, y, mass);
        if(!stationary)
        {
            sim_random_vector(rng_vel, 0, 1);
            vec2_assign(body->vel, rng_vel);
        }
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

void sim_create_random_distribution(Sim *sim, sfUint32 count, sfBool stationary)
{
    srand((unsigned int)time(NULL));
    mfloat_t pos[] = {sim_random_uint(0, WIN_WIDTH), sim_random_uint(0, WIN_HEIGHT)};
    mfloat_t rng_vel[VEC2_SIZE];
    Body *body = NULL;
    for(size_t i = 0; i < count; i++)
    {
        body = body_create(&sim->display, sim->bodies, &sim->num_of_bodies, pos[0], pos[1], BODY_DEFAULT_MASS);
        if(!stationary)
        {
            sim_random_vector(rng_vel, 0, 1);
            vec2_assign(body->vel, rng_vel);
        }
        pos[0] = sim_random_uint(0, WIN_WIDTH);
        pos[1] = sim_random_uint(0, WIN_HEIGHT);
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
    editor_destroy(&sim->editor);
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
    sfText_destroy(sim->zoom_text);
    sfText_destroy(sim->possible_collisions_text);
    sfClock_destroy(sim->delta_clock);
    sfView_destroy(sim->display.view);
    sfView_destroy(sim->display.gui_view);
    sfRenderWindow_destroy(sim->display.render_window);
    free(sim);
}