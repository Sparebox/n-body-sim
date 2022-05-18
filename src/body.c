#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "body.h"
#include "sim.h"
#include "trail.h"
#include "SFML/Graphics.h"

void runge_kutta_integrate_vel(Body *body, mfloat_t *new_vel, float delta_time);

void body_update(
    Body *body,
    Body *bodies,
    float delta_time
)
{
    // Gravity
    mfloat_t pos_a[VEC2_SIZE];
    mfloat_t pos_b[VEC2_SIZE];
    mfloat_t gravitation_force[VEC2_SIZE];
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape != NULL && bodies[i].shape != body->shape)
        {
            body_get_position(body, pos_a);
            body_get_position(&bodies[i], pos_b);
            const float distance2 = vec2_distance_squared(pos_a, pos_b);
            if(distance2 > DISTANCE_THRESHOLD * DISTANCE_THRESHOLD &&
                body->mass < BODY_DEFAULT_MASS * 2 &&
                bodies[i].mass < BODY_DEFAULT_MASS * 2)
            {
                continue;
            }
            body_calculate_gravitation_force(gravitation_force, body, &bodies[i], distance2);
            body_apply_force(body, gravitation_force);
        }
    }
    const sfVector2f pos = sfCircleShape_getPosition(body->shape);
    trail_append(&body->trail, pos.x, pos.y);
    // Runge Kutta 4 integration
    mfloat_t new_vel[VEC2_SIZE];
    runge_kutta_integrate_vel(body, new_vel, delta_time);
    //Limit speed
    if(vec2_length_squared(body->vel) > BODY_SPEED_LIMIT * BODY_SPEED_LIMIT && VELOCITY_LIMITED)
    {
        vec2_normalize(body->vel, body->vel);
        vec2_multiply_f(body->vel, body->vel, BODY_SPEED_LIMIT);
    }
    // Move body 
    const sfVector2f offset = {
        new_vel[0],
        new_vel[1]
    };
    sfCircleShape_move(body->shape, offset);
    sfText_move(body->info_text, offset);
}

void runge_kutta_integrate_vel(Body *body, mfloat_t *new_vel, float delta_time)
{
    mfloat_t k1[VEC2_SIZE];
    mfloat_t k2[VEC2_SIZE];
    mfloat_t k3[VEC2_SIZE];
    mfloat_t k4[VEC2_SIZE];

    vec2_add(body->vel, body->vel, body->acc);
    vec2_zero(body->acc);
    vec2_assign(k1, body->vel);
    vec2_multiply_f(k2, k1, delta_time / 2.f);
    vec2_add(k2, body->vel, k2);
    vec2_multiply_f(k3, k2, delta_time / 2.f);
    vec2_add(k3, body->vel, k3);
    vec2_multiply_f(k4, k3, delta_time);
    vec2_add(k4, body->vel, k4);

    vec2_add(new_vel, k1, k4);
    vec2_add(new_vel, vec2_multiply_f(k2, k2, 2.f), vec2_multiply_f(k3, k3, 2.f));
    vec2_multiply_f(new_vel, new_vel, delta_time / 6.f);
}

int body_compare_x_axis(const void *a, const void *b)
{
    Body **A = ((Body**)a);
    Body **B = ((Body**)b);
    if(*A == NULL && *B == NULL)
    {
        return 0;
    }
    if(*A == NULL)
    {
        return 1;
    }
    if(*B == NULL)
    {
        return -1;
    }
    const sfVector2f a_pos = sfCircleShape_getPosition((*A)->shape);
    const sfVector2f b_pos = sfCircleShape_getPosition((*B)->shape);
    return (a_pos.x - b_pos.x);
}

sfUint32 body_sweep_and_prune(Body *bodies, Body **possible_collisions)
{
    memset(possible_collisions, 0, MAX_BODIES * sizeof(NULL));
    Body *sorted_bodies[MAX_BODIES] = { 0 };
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape != NULL)
        {
            sorted_bodies[i] = &bodies[i];
        }
    }
    qsort(sorted_bodies, MAX_BODIES, sizeof(Body*), body_compare_x_axis); // Sort bodies by x-axis
    Body *active_intervals[MAX_BODIES] = { 0 };
    Body *last_active = NULL;
    active_intervals[0] = sorted_bodies[0];
    size_t interval_index = 0;
    size_t possible_index = 0;
    sfFloatRect a_bounds = { 0 };
    sfFloatRect b_bounds = { 0 };
    for(size_t i = 1; i < MAX_BODIES - 1; i++)
    {
        if(sorted_bodies[i] == NULL)
        {
            break;
        }
        for(size_t j = 0; j < MAX_BODIES; j++) // Go through active intervals
        {
            if(active_intervals[j] == NULL || active_intervals[j]->shape == NULL)
            {
                break;
            }
            a_bounds = sfCircleShape_getGlobalBounds(active_intervals[j]->shape);
            b_bounds = sfCircleShape_getGlobalBounds(sorted_bodies[i]->shape);
            b_bounds.top = a_bounds.top;
            if(sfFloatRect_intersects(&a_bounds, &b_bounds, NULL)) // If collision is possible
            {
                if(last_active != active_intervals[j]) // Check for overlap
                {
                    possible_collisions[possible_index] = active_intervals[j];
                    possible_collisions[possible_index + 1] = sorted_bodies[i];
                    possible_index += 2;
                }
                else
                {
                    possible_collisions[possible_index] = sorted_bodies[i];
                    possible_index++;
                }
                interval_index++;
                active_intervals[interval_index] = sorted_bodies[i];
                last_active = active_intervals[j];
                break;
            }
            else if(active_intervals[j + 1] == NULL)
            {
                memset(active_intervals, 0, MAX_BODIES * sizeof(NULL));
                interval_index = 0;
                active_intervals[interval_index] = sorted_bodies[i];
                break;
            }
        }
    }
    return (sfUint32) possible_index;
}

void body_handle_collision(Display *display, Body *a, Body *b, Body *bodies, sfUint32 *num_of_bodies)
{
    mfloat_t impact_dir[VEC2_SIZE];
    mfloat_t a_pos[VEC2_SIZE];
    mfloat_t b_pos[VEC2_SIZE];
    body_get_position(a, a_pos);
    body_get_position(b, b_pos);
    vec2_subtract(impact_dir, a_pos, b_pos);
    vec2_normalize(impact_dir, impact_dir);
    
    mfloat_t vel_diff[VEC2_SIZE];
    vec2_subtract(vel_diff, a->vel, b->vel);
    const float speed_diff2 = vec2_length_squared(vel_diff);
    Body *major_mass = a->mass > b->mass ? a : b; // if mass A is bigger then split A else split B
    Body *minor_mass = major_mass == a ? b : a;
    if(speed_diff2 > BODY_SPLIT_VELOCITY * BODY_SPLIT_VELOCITY &&
        major_mass->mass > 1 &&
        abs(minor_mass->mass - major_mass->mass) < 100e3 && 
        BODY_SPLITTING_ENABLED)
    {
        mfloat_t m1v1[VEC2_SIZE];
        mfloat_t m2v2[VEC2_SIZE];
        mfloat_t new_vel[VEC2_SIZE];
        mfloat_t separating_vel[VEC2_SIZE];
        
        vec2_multiply_f(m1v1, minor_mass->vel, minor_mass->mass);
        vec2_multiply_f(m2v2, major_mass->vel, major_mass->mass);
        vec2_add(new_vel, m1v1, m2v2);
        vec2_divide_f(new_vel, new_vel, minor_mass->mass + major_mass->mass);
        const sfVector2f major_mass_pos = sfCircleShape_getPosition(major_mass->shape);
        Body *new1 = body_create(
            display, bodies,
            num_of_bodies,
            major_mass_pos.x,
            major_mass_pos.y,
            (major_mass->mass + minor_mass->mass) / 2
            );
        Body *new2 = body_create(display,
            bodies,
            num_of_bodies,
            major_mass_pos.x,
            major_mass_pos.y,
            (major_mass->mass + minor_mass->mass) / 2
            );
        sfVector2f offset = { 0 };
        const float major_mass_radius = sfCircleShape_getRadius(major_mass->shape);
        vec2_rotate(separating_vel, impact_dir, MPI_2);
        offset.x = separating_vel[0] * major_mass_radius * 2;
        offset.y = separating_vel[1] * major_mass_radius * 2;
        sfCircleShape_move(new1->shape, offset);
        vec2_add(new1->vel, separating_vel, new_vel);
        vec2_rotate(separating_vel, impact_dir, -MPI_2);
        offset.x = separating_vel[0] * major_mass_radius * 2;
        offset.y = separating_vel[1] * major_mass_radius * 2;
        sfCircleShape_move(new2->shape, offset);
        vec2_add(new2->vel, separating_vel, new_vel);
        body_destroy(major_mass, bodies, num_of_bodies);
        body_destroy(minor_mass, bodies, num_of_bodies);
    }
    else // Bodies combine into one
    {
        mfloat_t m1v1[VEC2_SIZE];
        mfloat_t m2v2[VEC2_SIZE];
        mfloat_t new_vel[VEC2_SIZE];
        vec2_multiply_f(m1v1, a->vel, a->mass);
        vec2_multiply_f(m2v2, b->vel, b->mass);
        vec2_add(new_vel, m1v1, m2v2);
        vec2_divide_f(new_vel, new_vel, a->mass + b->mass);
        vec2_assign(major_mass->vel, new_vel);
        body_apply_mass(major_mass, a->mass + b->mass);
        // Calculate new color
        sfUint16 red_average = sfCircleShape_getFillColor(a->shape).r;
        red_average += sfCircleShape_getFillColor(b->shape).r;
        red_average /= 2;
        sfUint16 blue_average = sfCircleShape_getFillColor(a->shape).b;
        blue_average += sfCircleShape_getFillColor(b->shape).b;
        blue_average /= 2;
        sfUint16 green_average = sfCircleShape_getFillColor(a->shape).g;
        green_average += sfCircleShape_getFillColor(b->shape).g;
        green_average /= 2;
        const sfColor new_color = sfColor_fromRGB(red_average, green_average, blue_average);
        sfCircleShape_setFillColor(major_mass->shape, new_color);
        major_mass->trail.color = new_color;

        body_destroy(major_mass == a ? b : a, bodies, num_of_bodies);
    }
}

void body_check_collisions(Display *display, Body **possible_collisions, Body *bodies, sfUint32 *num_of_bodies)
{
    Body *a = NULL;
    Body *b = NULL;
    mfloat_t pos_a[VEC2_SIZE];
    mfloat_t pos_b[VEC2_SIZE];
    float distance2 = 0.f;
    float radius_a = 0.f;
    float radius_b = 0.f;
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        a = possible_collisions[i];
        if(a == NULL || a->shape == NULL)
        {
            break;
        }
        for(size_t j = 0; j < MAX_BODIES; j++)
        {
            b = possible_collisions[j];
            if(b == NULL || b->shape == NULL)
            {
                break;
            }
            else if(a == b)
            {
                continue;
            }
            body_get_position(a, pos_a);
            body_get_position(b, pos_b);
            distance2 = vec2_distance_squared(pos_a, pos_b);
            radius_a = sfCircleShape_getRadius(a->shape);
            radius_b = sfCircleShape_getRadius(b->shape);
            if(distance2 < (radius_a + radius_b) * (radius_a + radius_b))
            {
                body_handle_collision(display, a, b, bodies, num_of_bodies);
                break;
            }
        }
    }
}

void body_apply_force(Body *body, mfloat_t *force) 
{   
    vec2_divide_f(force, force, body->mass);
    vec2_add(body->acc, body->acc, force);
}

void body_apply_mass(Body *body, sfUint32 mass)
{
    body->mass = mass;
    const float radius = mass / BODY_RADIUS_FACTOR;
    sfCircleShape_setRadius(body->shape, radius);
    const sfVector2f origin = {radius, radius};
    sfCircleShape_setOrigin(body->shape, origin);
    // Update info text
    char info_string[32];
    sprintf(info_string, "Mass: %d", mass);
    sfText_setString(body->info_text, info_string);
    const sfFloatRect bounds = sfText_getGlobalBounds(body->info_text);
    sfVector2f pos = sfCircleShape_getPosition(body->shape);
    pos.x -= bounds.width / 2;
    sfText_setPosition(body->info_text, pos);
}

void body_calculate_gravitation_force(mfloat_t *result, Body *a, Body *b,  float dist2)
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

void body_get_position(Body *body, mfloat_t *result)
{
    const sfVector2f pos = sfCircleShape_getPosition(body->shape);
    result[0] = pos.x;
    result[1] = pos.y;
}


void body_render(Display *display, Body *body)
{
    const sfVector2i screen_pos = 
        sfRenderWindow_mapCoordsToPixel(display->render_window, sfCircleShape_getPosition(body->shape), display->view);
    const sfIntRect viewport = {0, 0, WIN_WIDTH, WIN_HEIGHT};
    if(!sfIntRect_contains(&viewport, screen_pos.x, screen_pos.y)) // Body is not in window
    {
        return;
    }
    trail_render(display, &body->trail);
    sfRenderWindow_drawCircleShape(display->render_window, body->shape, NULL);
    sfFloatRect text_bounds = { 0 };
    sfVector2f offset = { 0 };
    const sfVector2f text_scale = {display->zoom_level, display->zoom_level};
    const sfVector2f body_pos = sfCircleShape_getPosition(body->shape);
    const sfVector2f mouse_w_pos = 
        sfRenderWindow_mapPixelToCoords(display->render_window, sfMouse_getPositionRenderWindow(display->render_window), display->view);
    const sfFloatRect bounds = sfCircleShape_getGlobalBounds(body->shape);
    if(sfFloatRect_contains(&bounds, mouse_w_pos.x, mouse_w_pos.y))
    {
        sfText_setScale(body->info_text, text_scale);
        text_bounds = sfText_getGlobalBounds(body->info_text);
        offset.x = body_pos.x - text_bounds.width / 2;
        offset.y = body_pos.y;
        sfText_setPosition(body->info_text, offset);
        sfRenderWindow_drawText(display->render_window,        body->info_text, NULL);
    }
}

Body* body_create(Display *display, Body *bodies, sfUint32 *num_of_bodies, float x, float y, sfUint32 mass)
{
    assert(mass > 0);
    Body *body = NULL;
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape == NULL)
        {
            bodies[i].shape = sfCircleShape_create();
            bodies[i].info_text = sfText_create();
            body = &bodies[i];
            vec2_zero(body->vel);
            vec2_zero(body->acc);
            body_apply_mass(body, mass);
            break;
        }
    }
    if(body == NULL)
    {
        printf("Body array is full!\n");
        return NULL;
    }
    const sfVector2f pos = {x, y};
    const sfVector2f origin = {mass / BODY_RADIUS_FACTOR, mass / BODY_RADIUS_FACTOR};
    sfCircleShape_setOrigin(body->shape, origin);
    sfCircleShape_setPosition(body->shape, pos);
    const sfUint8 red = sim_random_uint(0, 255);
    const sfUint8 green = sim_random_uint(0, 255);
    const sfUint8 blue = sim_random_uint(0, 255);
    const sfColor color = sfColor_fromRGB(red, green, blue);
    sfCircleShape_setFillColor(body->shape, color);
    body->trail.color = color;
    body->trail.trail_timer = sfClock_create();
    body->trail.vertices[MAX_TRAIL_VERTICES - 1].position = pos;
    sfText_setPosition(body->info_text, pos);
    sfText_setOrigin(body->info_text, origin);
    sfText_setFont(body->info_text, display->font);
    sfText_setColor(body->info_text, sfRed);
    char info_string[32] = { 0 };
    sprintf(info_string, "Mass: %d", mass);
    sfText_setString(body->info_text, info_string);
    (*num_of_bodies)++;
    return body;
}

void body_destroy(Body *body, Body *bodies, sfUint32 *num_of_bodies)
{
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape == body->shape)
        {
            sfCircleShape_destroy(bodies[i].shape);
            bodies[i].shape = NULL;
            sfText_destroy(bodies[i].info_text);
            bodies[i].info_text = NULL;
            sfClock_destroy(bodies[i].trail.trail_timer);
            (*num_of_bodies)--;
            break;
        }
    }
}

void body_destroy_all(Body *bodies, sfUint32 *num_of_bodies)
{
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape != NULL)
        {
            sfCircleShape_destroy(bodies[i].shape);
            bodies[i].shape = NULL;
            sfText_destroy(bodies[i].info_text);
            bodies[i].info_text = NULL;
            sfClock_destroy(bodies[i].trail.trail_timer);
        }
    }
    (*num_of_bodies) = 0;
}