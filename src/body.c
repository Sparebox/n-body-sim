#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "body.h"
#include "sim.h"
#include "trail.h"

static void velocity_verlet_integerate(Body *body, mfloat_t *new_pos, const float delta_time);
static void solve_merge_collision(Body *a, Body *b, Body *bodies, sfUint32 *num_of_bodies);
static void solve_bounce_collision(Body *a, Body *b);
static int compare_x_axis(const void *a, const void *b);

void body_update(Body *body, const float delta_time)
{
    // Trail
    const sfVector2f pos = sfCircleShape_getPosition(body->shape);
    trail_append(&body->trail, pos.x, pos.y);
    // Velocity verlet integration
    mfloat_t new_pos_result[VEC2_SIZE];
    velocity_verlet_integerate(body, new_pos_result, delta_time);
    sfVector2f new_pos = {new_pos_result[0], new_pos_result[1]};
    //Limit speed
    if(vec2_length_squared(body->vel) > BODY_SPEED_LIMIT * BODY_SPEED_LIMIT && VELOCITY_LIMITED)
    {
        vec2_normalize(body->vel, body->vel);
        vec2_multiply_f(body->vel, body->vel, BODY_SPEED_LIMIT);
    }
    // Move body 
    sfCircleShape_setPosition(body->shape, new_pos);
    sfText_setPosition(body->info_text, new_pos);
}

void velocity_verlet_integerate(Body *body, mfloat_t *new_pos, const float delta_time)
{
    mfloat_t pos[VEC2_SIZE];
    mfloat_t temp[VEC2_SIZE];
    body_get_position(body, pos);

    // New position
    vec2_multiply_f(temp, body->vel, delta_time);
    vec2_add(new_pos, pos, temp);
    vec2_multiply_f(temp, body->acc, delta_time * delta_time * 0.5f);
    vec2_add(new_pos, new_pos, temp);
    // New velocity
    vec2_add(temp, body->acc, body->last_acc);
    vec2_multiply_f(temp, temp, delta_time * 0.5f);
    vec2_add(body->vel, temp, body->vel);

    vec2_assign(body->last_acc, body->acc);
    vec2_zero(body->acc);
}

int compare_x_axis(const void *a, const void *b)
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
    memset(possible_collisions, 0, MAX_BODIES * sizeof(Body*));
    Body *sorted_bodies[MAX_BODIES] = { 0 };
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape != NULL)
        {
            sorted_bodies[i] = &bodies[i];
        }
    }
    qsort(sorted_bodies, MAX_BODIES, sizeof(Body*), compare_x_axis); // Sort bodies by x-axis
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

void solve_merge_collision(Body *a, Body *b, Body *bodies, sfUint32 *num_of_bodies)
{
    mfloat_t a_pos[VEC2_SIZE];
    mfloat_t b_pos[VEC2_SIZE];
    body_get_position(a, a_pos);
    body_get_position(b, b_pos);
    Body *major_mass = a->mass > b->mass ? a : b; // if mass A is bigger then split A else split B
    
    // Bodies combine into one
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
    sfUint16 red_average = sfCircleShape_getFillColor(a->shape).r + sfCircleShape_getFillColor(b->shape).r;
    red_average /= 2.f;
    sfUint16 blue_average = sfCircleShape_getFillColor(a->shape).b + sfCircleShape_getFillColor(b->shape).b;
    blue_average /= 2.f;
    sfUint16 green_average = sfCircleShape_getFillColor(a->shape).g + sfCircleShape_getFillColor(b->shape).g;
    green_average /= 2.f;
    const sfColor new_color = sfColor_fromRGB(red_average, green_average, blue_average);
    sfCircleShape_setFillColor(major_mass->shape, new_color);
    major_mass->trail.color = new_color;

    body_destroy(major_mass == a ? b : a, bodies, num_of_bodies);
}

void solve_bounce_collision(Body *a, Body *b)
{
    mfloat_t a_pos[VEC2_SIZE];
    mfloat_t b_pos[VEC2_SIZE];
    mfloat_t normal[VEC2_SIZE];
    mfloat_t offset[VEC2_SIZE];
    // Remove intersection of bodies
    body_get_position(a, a_pos);
    body_get_position(b, b_pos);
    const float distance = vec2_distance(a_pos, b_pos);
    const float a_radius = sfCircleShape_getRadius(a->shape);
    const float b_radius = sfCircleShape_getRadius(b->shape);
    vec2_subtract(normal, a_pos, b_pos);
    vec2_normalize(normal, normal);
    vec2_multiply_f(offset, normal, (a_radius + b_radius - distance) / 2.f);
    vec2_add(a_pos, a_pos, offset);
    sfCircleShape_setPosition(a->shape, sim_to_sf_vector(a_pos));
    vec2_subtract(b_pos, b_pos, offset);
    sfCircleShape_setPosition(b->shape, sim_to_sf_vector(b_pos));
    // Apply impulse
    mfloat_t rel_vel[VEC2_SIZE];
    mfloat_t impulse_a[VEC2_SIZE];
    mfloat_t impulse_b[VEC2_SIZE];
    vec2_subtract(rel_vel, a->vel, b->vel);
    const float a_inv_mass = 1.f / a->mass;
    const float b_inv_mass = 1.f / b->mass;
    const float impulse = (-(1 + RESTITUTION_COEFF) * vec2_dot(rel_vel, normal)) / (a_inv_mass + b_inv_mass);
    vec2_multiply_f(impulse_a, normal, impulse / a->mass);
    vec2_multiply_f(impulse_b, normal, -impulse / b->mass);
    vec2_add(a->vel, a->vel, impulse_a);
    vec2_add(b->vel, b->vel, impulse_b);
}

void body_check_collisions(Body **possible_collisions, Body *bodies, sfUint32 *num_of_bodies, sfUint32 *collision_type)
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
                switch(*collision_type)
                {
                    case BOUNCY_COLLISIONS:
                        solve_bounce_collision(a, b);
                        break;
                    case MERGE_COLLISIONS:
                        solve_merge_collision(a, b, bodies, num_of_bodies);
                        break;
                }
                break;
            }
        }
    }
}

void body_apply_force(Body *body, const mfloat_t *force) 
{   
    mfloat_t new_acc[VEC2_SIZE];
    vec2_divide_f(new_acc, force, body->mass);
    vec2_add(body->acc, body->acc, new_acc);
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

void body_get_position(Body *body, mfloat_t *result)
{
    const sfVector2f pos = sfCircleShape_getPosition(body->shape);
    result[0] = pos.x;
    result[1] = pos.y;
}


void body_render(Display *display, Body *body, sfBool *editor_enabled)
{
    const sfVector2i screen_pos = 
        sfRenderWindow_mapCoordsToPixel(display->render_window, sfCircleShape_getPosition(body->shape), display->view);
    const sfIntRect viewport = {0, 0, WIN_WIDTH, WIN_HEIGHT};
    if(!sfIntRect_contains(&viewport, screen_pos.x, screen_pos.y)) // If body is not in window don't render
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
    // Show info text if mouse is on body and not in editor
    if(sfFloatRect_contains(&bounds, mouse_w_pos.x, mouse_w_pos.y) && !(*editor_enabled))
    {
        sfText_setScale(body->info_text, text_scale);
        text_bounds = sfText_getGlobalBounds(body->info_text);
        offset.x = body_pos.x - text_bounds.width / 2;
        offset.y = body_pos.y;
        sfText_setPosition(body->info_text, offset);
        sfRenderWindow_drawText(display->render_window, body->info_text, NULL);
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
        printf("Tried to overallocate body array!\n");
        return NULL;
    }
    body->moment_of_inertia = (2.f / 5.f) * mass * powf(mass / BODY_RADIUS_FACTOR, 2.f);
    const sfVector2f pos = {x, y};
    const sfVector2f origin = {mass / BODY_RADIUS_FACTOR, mass / BODY_RADIUS_FACTOR};
    sfCircleShape_setOrigin(body->shape, origin);
    sfCircleShape_setPosition(body->shape, pos);
    const sfUint8 red = sim_random_int(0, 255);
    const sfUint8 green = sim_random_int(0, 255);
    const sfUint8 blue = sim_random_int(0, 255);
    const sfColor color = sfColor_fromRGB(red, green, blue);
    sfCircleShape_setFillColor(body->shape, color);
    body->trail.color = color;
    body->trail.trail_timer = sfClock_create();
    body->trail.current_index = 0;
    memset(body->trail.vertices, 0, MAX_TRAIL_VERTICES * sizeof(sfVertex));
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