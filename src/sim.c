#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "sim.h"
#include "editor.h"

static void sat_find_min_separation_rect_rect(Rot_body *a, Rot_body *b, float *separation, mfloat_t *sep_normal);
static void sat_find_min_separation_circle_rect(Body *circle, Rot_body *rect, float *separation, mfloat_t *sep_normal, mfloat_t *point_of_contact);
static void handle_left_click(Sim *sim);
static void handle_mouse_scroll(Sim *sim, sfEvent *event);
static void init_gui(Sim *sim);
static void update_gui(void *sim);
static Body *possible_collisions[MAX_BODIES << 1];

static sfBool sim_is_destroyed = sfFalse;

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
                handle_mouse_scroll(sim, &event);
                break;
            case sfEvtMouseButtonPressed :
                display->last_mouse_click_pos = sfMouse_getPositionRenderWindow(window);
                if(event.mouseButton.button == sfMouseLeft)
                {
                    handle_left_click(sim);
                }
                else if(event.mouseButton.button == sfMouseRight)
                {
                    if(!sim->editor_enabled)
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
                        if(sim->editor.selected_body != NULL && !sim->editor.torque_mode_enabled)
                        {
                            editor_apply_velocity(&sim->editor, display);
                        }
                        if(sim->editor.circle_mode_enabled)
                        {
                            const sfVector2f world_mouse_pos = 
                                sfRenderWindow_mapPixelToCoords(display->render_window, display->last_mouse_click_pos, display->view);
                            sim_create_circle(
                                sim,
                                world_mouse_pos.x,
                                world_mouse_pos.y,
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
                        sim->editor.new_body_mass = BODY_DEFAULT_MASS;
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
                            sim->largest_body = NULL;
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
                    case sfKeyT:
                        sim->editor.torque_mode_enabled = !sim->editor.torque_mode_enabled;
                        break;
                    case sfKeyAdd :
                        sim->sim_speed_multiplier++;
                        break;
                    case sfKeySubtract :
                        if(sim->sim_speed_multiplier > 1)
                        {
                            sim->sim_speed_multiplier--;
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

void handle_mouse_scroll(Sim *sim, sfEvent *event)
{
    if(sim->editor_enabled)
    {
        sfFloatRect bounds = { 0 };
        const sfVector2i screen_mouse_pos = sfMouse_getPositionRenderWindow(sim->display.render_window);
        const sfVector2f world_mouse_pos = 
            sfRenderWindow_mapPixelToCoords(sim->display.render_window, screen_mouse_pos, sim->display.view);
        // Modifying an existing body
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
                    body_apply_mass(&sim->bodies[i], sim->bodies[i].mass + BODY_DEFAULT_MASS * sim->display.zoom_level * 10);
                }
                else if(sim->bodies[i].mass > BODY_DEFAULT_MASS * 2)
                {
                    body_apply_mass(&sim->bodies[i], sim->bodies[i].mass - BODY_DEFAULT_MASS * sim->display.zoom_level * 10);
                }
            }
        }
        // Creating a new body
        if(event->mouseWheelScroll.delta > 0.f)
        {
            sim->editor.new_body_mass += BODY_DEFAULT_MASS * sim->display.zoom_level;
        }
        else if(sim->editor.new_body_mass > BODY_DEFAULT_MASS * 2 )
        {
            sim->editor.new_body_mass -= BODY_DEFAULT_MASS * sim->display.zoom_level;
        }
        char new_mass_string[32];
        sprintf(new_mass_string, "MASS %d", sim->editor.new_body_mass);
        sfText_setString(sim->editor.new_body_mass_text, new_mass_string);
    }
    else // Zooming the view
    {
        sim->display.zoom_level = 
            event->mouseWheelScroll.delta > 0.f ? 1.1f * sim->display.zoom_level + 0.1f : 0.9f * sim->display.zoom_level - 0.1f;
        sim->display.zoom_level = sim->display.zoom_level < 0.1f ? 0.1f : sim->display.zoom_level;
        const sfVector2f new_size = {WIN_WIDTH * sim->display.zoom_level, WIN_HEIGHT * sim->display.zoom_level};
        sfView_setSize(sim->display.view, new_size);
    }
}

void handle_left_click(Sim *sim)
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
        else // Mouse was not on body
        {
            sim->editor.selected_body = NULL;
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
        else if(!sim->editor.torque_mode_enabled)
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

void sim_init(Sim *sim)
{
    sim->delta_clock = sfClock_create();
    sim->sim_speed_multiplier = 1;
    init_gui(sim);
}

void init_gui(Sim *sim)
{
    // Create separate thread for GUI updates
    sim->gui_update_thread = sfThread_create(update_gui, sim);
    sim->gui_mutex = sfMutex_create();
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
    editor_init_gui(&sim->editor, &sim->display);
    sfThread_launch(sim->gui_update_thread);
}

void update_gui(void *sim_data)
{
    Sim *sim =  (Sim*) sim_data;
    const sfTime sleep_duration = {.microseconds = 1.f/(30.f / 1e6)}; // 30 Updates per second
    while(!sim_is_destroyed)
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

        sprintf(string, "POSSIBLE COLLISIONS %d", sim->num_of_possible_collisions);
        sfText_setString(sim->possible_collisions_text, string);

        sprintf(string, sim->paused ? "PAUSED" : sim->editor_enabled ? "EDITOR" : "RUNNING");
        sfText_setString(sim->paused_text, string);
        
        if(sim->editor_enabled)
        {
            sfMutex_lock(sim->gui_mutex);
            editor_update(sim);
            sfMutex_unlock(sim->gui_mutex);
        }
        sfSleep(sleep_duration);
    }
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
    if(sim->paused || sim->editor_enabled)
    {
        return;
    }
    switch(sim->sim_type)
    {
        case GRAVITATIONAL_SIM:
            sim_apply_gravitation_forces(sim->bodies);
            break;
        case ATOMIC_FORCE_SIM:
            sim_apply_interatomic_forces(sim->bodies);
            break;
        case ROTATIONAL_PHYSICS_SIM:
            break;
    }
    sfUint32 largest_mass = 0;
    if(sim->largest_body != NULL)
    {
        largest_mass = sim->largest_body->mass;
    }
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL)
        {
            if(sim->bodies[i].mass > largest_mass)
            {
                sim->largest_body = &sim->bodies[i];
                largest_mass = sim->largest_body->mass;
            }
            body_update(
                &sim->bodies[i],
                sim->integrator,
                sfTime_asSeconds(sim->delta_time) * sim->sim_speed_multiplier
            );
        }
    }
    sim->num_of_possible_collisions = body_sweep_and_prune(sim->bodies, possible_collisions);
    body_check_collisions(possible_collisions, sim->bodies, &sim->num_of_bodies, &sim->collision_type);
}

void sim_render(Sim *sim)
{
    // Move view
    if(sim->following_largest_body && sim->largest_body != NULL && sim->largest_body->shape != NULL)
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
            body_render(&sim->display, &sim->bodies[i], &sim->editor_enabled);
        }
    }
    if(sim->editor_enabled)
    {
        sfMutex_lock(sim->gui_mutex);
        editor_render(sim);
        sfMutex_unlock(sim->gui_mutex);
        editor_render_gui(&sim->editor, &sim->display);
    }
    sim_render_gui(sim);
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
    sfBool give_random_vel)
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
        if(give_random_vel)
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

void sim_create_random_distribution(Sim *sim, sfUint32 count, sfBool give_random_vel)
{
    srand((unsigned int)time(NULL));
    const sfVector2f view_size = sfView_getSize(sim->display.view);
    const sfVector2f view_center = sfView_getCenter(sim->display.view);
    const float left    = view_center.x - view_size.x / 2.f;
    const float right   = view_center.x + view_size.x / 2.f;
    const float top     = view_center.y - view_size.y / 2.f;
    const float bottom  = view_center.y + view_size.y / 2.f;
    mfloat_t pos[] = {sim_random_int(left, right), sim_random_int(top, bottom)};
    mfloat_t rng_vel[VEC2_SIZE];
    Body *body = NULL;
    for(size_t i = 0; i < count; i++)
    {
        body = body_create(&sim->display, sim->bodies, &sim->num_of_bodies, pos[0], pos[1], BODY_DEFAULT_MASS);
        if(body == NULL)
        {
            return;
        }
        if(give_random_vel)
        {
            sim_random_vector(rng_vel, 0.f, 50.f);
            vec2_assign(body->vel, rng_vel);
        }
        pos[0] = sim_random_int(left, right);
        pos[1] = sim_random_int(top, bottom);
    }
}

sfInt32 sim_random_int(sfInt32 min, sfInt32 max)
{
    return rand() % (max - min + 1) + min;
}

float sim_random_float(float min, float max)
{
    const float random = ((float) rand() / (float) RAND_MAX) * max;
    return random < min ? min : random;
}

void sim_random_vector(mfloat_t *result, sfUint32 min_length, sfUint32 max_length)
{
    vec2_one(result);
    vec2_rotate(result, result, sim_random_float(0.f, 2 * M_PI));
    vec2_multiply_f(result, result, sim_random_int(min_length, max_length));
}

void sim_print_vector(const mfloat_t *vec)
{
    printf("Vec: [%.2f, %.2f]\n", vec[0], vec[1]);
}

void sim_print_sf_vector(const sfVector2f *vec)
{
    printf("Vec: [%.2f, %.2f]\n", vec->x, vec->y);
}

void sim_apply_gravitation_forces(Body *bodies)
{
    mfloat_t pos_a[VEC2_SIZE];
    mfloat_t pos_b[VEC2_SIZE];
    mfloat_t gravity[VEC2_SIZE];
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape == NULL)
        {
            continue;
        }
        for(size_t j = 0; j < MAX_BODIES - 1; j++)
        {
            if((bodies[i].shape == bodies[j].shape) || bodies[j].shape == NULL)
            {
                continue;
            }
            body_get_position(&bodies[i], pos_a);
            body_get_position(&bodies[j], pos_b);
            const float distance2 = vec2_distance_squared(pos_a, pos_b);
            if(distance2 > DISTANCE_THRESHOLD * DISTANCE_THRESHOLD)
            {
                continue;
            }
            sim_gravitation_force(gravity, &bodies[i], &bodies[j], distance2);
            body_apply_force(&bodies[i], gravity);
            vec2_negative(gravity, gravity);
            body_apply_force(&bodies[j], gravity);
        }
    }
}

void sim_gravitation_force(mfloat_t *result, Body *a, Body *b, float dist2)
{
    if(dist2 == 0.f) // Prevent division by zero
    {
        return;
    }
    mfloat_t a_pos[VEC2_SIZE];
    mfloat_t b_pos[VEC2_SIZE];
    const float force = (GRAVITATIONAL_CONSTANT * a->mass * b->mass) / dist2;
    body_get_position(a, a_pos);
    body_get_position(b, b_pos);
    vec2_subtract(result, b_pos, a_pos);
    vec2_normalize(result, result);
    vec2_multiply_f(result, result, force);
}

void sim_apply_interatomic_forces(Body *bodies)
{
    mfloat_t pos_a[VEC2_SIZE];
    mfloat_t pos_b[VEC2_SIZE];
    mfloat_t direction[VEC2_SIZE];
    mfloat_t waals_force[VEC2_SIZE];
    mfloat_t coulomb_force[VEC2_SIZE];
    mfloat_t resultant_force[VEC2_SIZE];
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape == NULL)
        {
            continue;
        }
        for(size_t j = 0; j < MAX_BODIES - 1; j++)
        {
            if((bodies[i].shape == bodies[j].shape) || bodies[j].shape == NULL)
            {
                continue;
            }
            body_get_position(&bodies[i], pos_a);
            body_get_position(&bodies[j], pos_b);
            const float distance2 = vec2_distance_squared(pos_a, pos_b);
            if(distance2 > DISTANCE_THRESHOLD * DISTANCE_THRESHOLD)
            {
                continue;
            }
            vec2_subtract(direction, pos_a, pos_b);
            vec2_normalize(direction, direction);
            sim_coulombic_force(coulomb_force, &bodies[i], &bodies[j], distance2, direction); // Attractive force
            vec2_negative(coulomb_force, coulomb_force);
            sim_van_der_waals_force(waals_force, &bodies[i], &bodies[j], distance2, direction); // Repulsive force
            vec2_add(resultant_force, coulomb_force, waals_force);
            body_apply_force(&bodies[i], resultant_force);
            vec2_negative(resultant_force, resultant_force);
            body_apply_force(&bodies[j], resultant_force);
        }
    }
}

void sim_coulombic_force(mfloat_t *result, Body *a, Body *b, float dist2, mfloat_t *direction)
{
    const float magnitude = (a->mass * b->mass) / (dist2 * 4 * MPI * EPSILON_0);
    vec2_multiply_f(result, direction, magnitude);
}

void sim_van_der_waals_force(mfloat_t *result, Body *a, Body *b, float dist2, mfloat_t *direction)
{
    const float a_radius = sfCircleShape_getRadius(a->shape);
    const float b_radius = sfCircleShape_getRadius(b->shape);
    const float magnitude = (HAMAKER_COEFF * 64 * powf(a_radius, 3) * powf(b_radius, 3) * sqrtf(dist2))
                            / (6 * (powf(dist2 - powf(a_radius + b_radius, 2), 2) * powf(dist2 - powf(a_radius - b_radius, 2), 2)));
    vec2_multiply_f(result, direction, magnitude);
}

sfVector2f sim_to_sf_vector(mfloat_t *vec2)
{
    return (sfVector2f) {vec2[0], vec2[1]};
}

void sim_from_sf_vector(mfloat_t *result, const sfVector2f sf_vec)
{
    result[0] = sf_vec.x;
    result[1] = sf_vec.y;
}

sfVector2f sim_closest_point_to_line(const sfVector2f _pos, const sfVector2f _a, const sfVector2f _b)
{
    mfloat_t pos[] = {_pos.x, _pos.y};
    mfloat_t a[] = {_a.x, _a.y};
    mfloat_t b[] = {_b.x, _b.y};
    mfloat_t line_normal[VEC2_SIZE];
    mfloat_t a_to_b[VEC2_SIZE];
    vec2_subtract(a_to_b, b, a);
    vec2_normalize(line_normal, a_to_b);
    mfloat_t pos_to_a[VEC2_SIZE];
    vec2_subtract(pos_to_a, a, pos);
    mfloat_t pos_to_b[VEC2_SIZE];
    vec2_subtract(pos_to_b, b, pos);
    const float distance_from_a = vec2_dot(pos_to_a, line_normal);
    if(distance_from_a > 0.f)
    {
        return _a;
    }
    const float distance_from_b = vec2_dot(pos_to_b, line_normal);
    if(distance_from_b < 0.f)
    {
        return _b;
    }
    mfloat_t closest_point[VEC2_SIZE];
    vec2_multiply_f(closest_point, line_normal, distance_from_a);
    vec2_subtract(closest_point, pos_to_a, closest_point);
    vec2_add(closest_point, closest_point, pos);
    return (sfVector2f) {closest_point[0], closest_point[1]};
}

void sim_get_normal(mfloat_t *result, const mfloat_t *point_a, const mfloat_t *point_b)
{
    mfloat_t a_to_b[VEC2_SIZE];
    vec2_subtract(a_to_b, point_b, point_a);
    vec2_tangent(result, a_to_b);
    vec2_normalize(result, result);
}

// Separating axis theorem collision resolution
void sim_sat_collision_resolution_rect_rect(Rot_body *a, Rot_body *b)
{
    float separation_a = 0.f;
    float separation_b = 0.f;
    mfloat_t normal_a[VEC2_SIZE];
    mfloat_t normal_b[VEC2_SIZE];
    mfloat_t offset[VEC2_SIZE];
    sat_find_min_separation_rect_rect(a, b, &separation_a, normal_a);
    sat_find_min_separation_rect_rect(b, a, &separation_b, normal_b);
    if(separation_a > 0.f || separation_b > 0.f)
    {
        return; // No collision
    }
    if(separation_a < separation_b)
    {
        vec2_multiply_f(offset, normal_a, separation_a / 2.f);
    }
    else
    {
        vec2_multiply_f(offset, normal_b, separation_b / 2.f);
    }
    sfRectangleShape_move(a->shape, sim_to_sf_vector(offset));
    vec2_negative(offset, offset);
    sfRectangleShape_move(b->shape, sim_to_sf_vector(offset));
}

void sat_find_min_separation_rect_rect(Rot_body *a, Rot_body *b, float *result, mfloat_t *sep_normal)
{
    float separation = -INFINITY;
    float min_separation = INFINITY;
    mfloat_t normal[VEC2_SIZE];
    mfloat_t point_a[VEC2_SIZE];
    mfloat_t point_b[VEC2_SIZE];
    mfloat_t point_c[VEC2_SIZE];
    mfloat_t diff[VEC2_SIZE];
    const sfTransform a_transform = sfRectangleShape_getTransform(a->shape);
    const sfTransform b_transform = sfRectangleShape_getTransform(b->shape);
    for(size_t i = 0; i < 4; i++)
    {
        sim_from_sf_vector(point_a, sfTransform_transformPoint(&a_transform, sfRectangleShape_getPoint(a->shape, i)));
        sim_from_sf_vector(point_b, sfTransform_transformPoint(&a_transform, sfRectangleShape_getPoint(a->shape, (i + 1) % 4)));
        sim_get_normal(normal, point_a, point_b);
        min_separation = INFINITY;
        for(size_t j = 0; j < 4; j++)
        {
            sim_from_sf_vector(point_c, sfTransform_transformPoint(&b_transform, sfRectangleShape_getPoint(b->shape, j)));
            vec2_subtract(diff, point_c, point_b);
            min_separation = fminf(min_separation, vec2_dot(diff, normal));
        }
        if(min_separation > separation)
        {
            separation = min_separation;
            (*result) = separation;
            vec2_assign(sep_normal, normal);
        }
    }
}

void sim_collision_resolution_circle_rect(Body *circle, Rot_body *rect)
{
    float separation = 0.f;
    mfloat_t normal[VEC2_SIZE];
    mfloat_t offset[VEC2_SIZE];
    mfloat_t point_of_contact[VEC2_SIZE];
    sat_find_min_separation_circle_rect(circle, rect, &separation, normal, point_of_contact);
    if(separation < 0.f)
    {
        return;
    }
    // Move bodies apart
    vec2_multiply_f(offset, normal, separation / 2.f);
    sfCircleShape_move(circle->shape, sim_to_sf_vector(offset));
    vec2_negative(offset, offset);
    sfRectangleShape_move(rect->shape, sim_to_sf_vector(offset));

    mfloat_t rel_vel[VEC2_SIZE];
    mfloat_t rect_pos[VEC2_SIZE];
    mfloat_t circle_pos[VEC2_SIZE];
    mfloat_t moment_arm_circle[] = {0.f, 0.f, 0.f};
    mfloat_t moment_arm_rect[] = {0.f, 0.f, 0.f};
    mfloat_t circle_cross[VEC3_SIZE];
    mfloat_t rect_cross[VEC3_SIZE];
    const float circle_inv_mass = 1.f / circle->mass;
    const float rect_inv_mass = 1.f / rect->mass;

    body_get_position(circle, circle_pos);
    sim_from_sf_vector(rect_pos, sfRectangleShape_getPosition(rect->shape));
    vec2_subtract(moment_arm_circle, point_of_contact, circle_pos);
    vec2_subtract(moment_arm_rect, point_of_contact, rect_pos);
    vec2_subtract(rel_vel, circle->vel, rect->vel);
    vec3_cross(circle_cross, moment_arm_circle, normal);
    vec3_cross(rect_cross, moment_arm_rect, normal);
    float denominator = circle_inv_mass + rect_inv_mass;
    denominator += powf(circle_cross[2], 2.f) / circle->moment_of_inertia;
    denominator += powf(rect_cross[2], 2.f) / rect->moment_of_inertia;
    const float nominator = -(1 + RESTITUTION_COEFF) * vec2_dot(rel_vel, normal);
    const float impulse = nominator / denominator;
    mfloat_t circle_linear_impulse[VEC2_SIZE];
    mfloat_t rect_linear_impulse[VEC2_SIZE];

    vec2_multiply_f(circle_linear_impulse, normal, impulse / circle->mass);
    vec2_multiply_f(rect_linear_impulse, normal, -impulse / rect->mass);
    vec2_add(circle->vel, circle->vel, circle_linear_impulse);
    vec2_add(rect->vel, rect->vel, rect_linear_impulse);

   mfloat_t rect_angular_impulse[VEC2_SIZE];
   vec2_multiply_f(rect_angular_impulse, normal, -impulse);
   vec3_cross(rect_cross, moment_arm_rect, rect_angular_impulse);
   rect->angular_vel += rect_cross[2] / rect->moment_of_inertia;
}

void sat_find_min_separation_circle_rect(Body *circle, Rot_body *rect, float *result, mfloat_t *sep_normal, mfloat_t *point_of_contact)
{
    float min_separation = INFINITY;
    float separation = 0.f;
    mfloat_t normal[VEC2_SIZE];
    mfloat_t point_a[VEC2_SIZE];
    mfloat_t point_b[VEC2_SIZE];
    mfloat_t circle_pos[VEC2_SIZE];
    mfloat_t diff[VEC2_SIZE];
    mfloat_t circle_surface[VEC2_SIZE];
    const sfTransform rect_transform = sfRectangleShape_getTransform(rect->shape);
    body_get_position(circle, circle_pos);
    for(size_t j = 0; j < 4; j++)
    {
        sim_from_sf_vector(point_a, sfTransform_transformPoint(&rect_transform, sfRectangleShape_getPoint(rect->shape, j)));
        sim_from_sf_vector(point_b, sfTransform_transformPoint(&rect_transform, sfRectangleShape_getPoint(rect->shape, (j + 1) % 4)));
        sim_get_normal(normal, point_a, point_b);
        vec2_multiply_f(circle_surface, normal, sfCircleShape_getRadius(circle->shape));
        vec2_subtract(circle_surface, circle_pos, circle_surface);
        vec2_subtract(diff, point_b, circle_surface);
        separation = vec2_dot(diff, normal);
        if(separation < min_separation)
        {
            min_separation = separation;
            vec2_assign(sep_normal, normal);
            vec2_assign(point_of_contact, circle_surface);
        }
    }
    (*result) = min_separation;
}

void sim_destroy(Sim *sim)
{
    sim_is_destroyed = sfTrue;
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
    sfText_destroy(sim->paused_text);
    sfClock_destroy(sim->delta_clock);
    display_destroy(&sim->display);
    sfThread_destroy(sim->gui_update_thread);
    sfMutex_destroy(sim->gui_mutex);
    free(sim);
}