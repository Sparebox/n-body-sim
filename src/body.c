#include <stdio.h>
#include "body.h"
#include "sim.h"
#include "trail.h"
#include "SFML/Graphics.h"

void body_update(Display *display, Body *body, Body *bodies, sfUint32 *num_of_bodies, float delta_time)
{
    // Gravity
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].id != body->id && bodies[i].shape != NULL)
        {
            mfloat_t pos_a[VEC2_SIZE];
            mfloat_t pos_b[VEC2_SIZE];
            body_get_position(body, pos_a);
            body_get_position(&bodies[i], pos_b);
            float distance2 = vec2_distance_squared(pos_a, pos_b);
            if(body->mass < BODY_RADIUS_FACTOR && bodies[i].mass < BODY_RADIUS_FACTOR && distance2 > 300 * 300)
            {
                continue;
            }
            float radius_a = sfCircleShape_getRadius(body->shape);
            float radius_b = sfCircleShape_getRadius(bodies[i].shape);
            if(distance2 < (radius_a + radius_b) * (radius_a + radius_b))
            {
                body_handle_collision(display, body, &bodies[i], bodies, num_of_bodies);
                return;
            }
            const float force = body_calculate_gravitation_force(body, &bodies[i]);
            mfloat_t dir[VEC2_SIZE];
            vec2_subtract(dir, pos_b, pos_a);
            vec2_normalize(dir, dir);
            vec2_multiply_f(dir, dir, force);
            body_apply_force(body, dir);
        }
    }
    //
    const sfVector2f pos = sfCircleShape_getPosition(body->shape);
    trail_append(&body->trail, pos.x, pos.y);

    // Integration
    vec2_multiply_f(body->acc, body->acc, delta_time);
    vec2_add(body->vel, body->vel, body->acc);
    vec2_zero(body->acc);
    //Limit speed
    if(vec2_length_squared(body->vel) > BODY_SPEED_LIMIT * BODY_SPEED_LIMIT && VELOCITY_LIMITED)
    {
        vec2_normalize(body->vel, body->vel);
        vec2_multiply_f(body->vel, body->vel, BODY_SPEED_LIMIT);
    }
    const sfVector2f offset = {body->vel[0] * delta_time, body->vel[1] * delta_time};
    sfCircleShape_move(body->shape, offset);
    sfText_move(body->info_text, offset);
}

void body_handle_collision(Display *display, Body *a, Body *b, Body *bodies, sfUint32 *num_of_bodies)
{
    float a_vel2 = vec2_length_squared(a->vel);
    float b_vel2 = vec2_length_squared(b->vel);
    mfloat_t impact_dir[VEC2_SIZE];
    mfloat_t a_pos[VEC2_SIZE];
    mfloat_t b_pos[VEC2_SIZE];
    body_get_position(a, a_pos);
    body_get_position(b, b_pos);
    vec2_subtract(impact_dir, a_pos, b_pos);
    vec2_normalize(impact_dir, impact_dir);
    if(a_vel2 == BODY_SPEED_LIMIT * BODY_SPEED_LIMIT && b->mass > a->mass) // Body b is going to split
    {
        printf("B splitting!\n");
        mfloat_t m1v1[VEC2_SIZE];
        mfloat_t m1v2[VEC2_SIZE];
        mfloat_t m2v2[VEC2_SIZE];
        mfloat_t new_a_vel[VEC2_SIZE];
        vec2_multiply_f(m1v1, b->vel, 2 * b->mass);
        vec2_multiply_f(m1v2, a->vel, b->mass);
        vec2_multiply_f(m2v2, a->vel, a->mass);
        vec2_subtract(new_a_vel, m1v1, m1v2);
        vec2_add(new_a_vel, new_a_vel, m2v2);
        vec2_divide_f(new_a_vel, new_a_vel, a->mass + b->mass);

        mfloat_t m2v1[VEC2_SIZE];
        mfloat_t new_b_vel[VEC2_SIZE];
        vec2_multiply_f(m2v1, b->vel, a->mass);
        vec2_subtract(new_b_vel, m1v1, m2v1);
        vec2_multiply_f(m2v2, m2v2, 2);
        vec2_add(new_b_vel, new_b_vel, m2v2);
        vec2_divide_f(new_b_vel, new_b_vel, a->mass + b->mass);

        vec2_assign(a->vel, new_a_vel);

        const sfVector2f b_pos = sfCircleShape_getPosition(b->shape);
        vec2_rotate(impact_dir, impact_dir, MPI_2);
        vec2_multiply_f(impact_dir, impact_dir, 2 * sfCircleShape_getRadius(b->shape));
        body_destroy(b, bodies, num_of_bodies);
        Body *new1 = body_create(display, bodies, num_of_bodies, b_pos.x + impact_dir[0], b_pos.y + impact_dir[1], b->mass / 2);
        vec2_assign(new1->vel, new_b_vel);
        vec2_rotate(impact_dir, impact_dir, MPI);
        Body *new2 = body_create(display, bodies, num_of_bodies, b_pos.x + impact_dir[0], b_pos.y + impact_dir[1], b->mass / 2);
        vec2_assign(new2->vel, new_b_vel);
    }
    else if(b_vel2 == BODY_SPEED_LIMIT * BODY_SPEED_LIMIT && a->mass > b->mass) // Body a is going to split
    {
        mfloat_t m1v1[VEC2_SIZE];
        mfloat_t m1v2[VEC2_SIZE];
        mfloat_t m2v2[VEC2_SIZE];
        mfloat_t new_a_vel[VEC2_SIZE];
        vec2_multiply_f(m1v1, a->vel, 2 * a->mass);
        vec2_multiply_f(m1v2, b->vel, a->mass);
        vec2_multiply_f(m2v2, b->vel, b->mass);
        vec2_subtract(new_a_vel, m1v1, m1v2);
        vec2_add(new_a_vel, new_a_vel, m2v2);
        vec2_divide_f(new_a_vel, new_a_vel, a->mass + b->mass);

        mfloat_t m2v1[VEC2_SIZE];
        mfloat_t new_b_vel[VEC2_SIZE];
        vec2_multiply_f(m2v1, a->vel, b->mass);
        vec2_subtract(new_b_vel, m1v1, m2v1);
        vec2_multiply_f(m2v2, m2v2, 2);
        vec2_add(new_b_vel, new_b_vel, m2v2);
        vec2_divide_f(new_b_vel, new_b_vel, a->mass + b->mass);

        vec2_assign(b->vel, new_b_vel);

        const sfVector2f a_pos = sfCircleShape_getPosition(a->shape);
        vec2_rotate(impact_dir, impact_dir, MPI_2);
        vec2_multiply_f(impact_dir, impact_dir, 2 * sfCircleShape_getRadius(a->shape));
        body_destroy(a, bodies, num_of_bodies);
        Body *new1 = body_create(display, bodies, num_of_bodies, a_pos.x + impact_dir[0], a_pos.y + impact_dir[1], a->mass / 2);
        vec2_assign(new1->vel, new_a_vel);
        vec2_rotate(impact_dir, impact_dir, MPI);
        Body *new2 = body_create(display, bodies, num_of_bodies, a_pos.x + impact_dir[0], a_pos.y + impact_dir[1], a->mass / 2);
        vec2_assign(new2->vel, new_a_vel);
    }
    else
    {
        Body *survivor = a->mass > b->mass ? a : b;
        mfloat_t mv_a[VEC2_SIZE];
        mfloat_t mv_b[VEC2_SIZE];
        vec2_assign(mv_a, a->vel);
        vec2_assign(mv_b, b->vel);
        vec2_multiply_f(mv_a, mv_a, a->mass);
        vec2_multiply_f(mv_b, mv_b, b->mass);
        mfloat_t new_vel[VEC2_SIZE];
        vec2_add(new_vel, mv_a, mv_b);
        vec2_divide_f(new_vel, new_vel, a->mass + b->mass);
        vec2_assign(survivor->vel, new_vel);
        body_apply_mass(survivor, a->mass + b->mass);
        body_destroy(survivor == a ? b : a, bodies, num_of_bodies);
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
    // Info text
    char info_string[32];
    sprintf(info_string, "Mass: %d", mass);
    sfText_setString(body->info_text, info_string);
    const sfFloatRect bounds = sfText_getGlobalBounds(body->info_text);
    sfVector2f pos = sfCircleShape_getPosition(body->shape);
    pos.x -= bounds.width / 2;
    sfText_setPosition(body->info_text, pos);
}

float body_calculate_gravitation_force(Body *a, Body *b)
{
    mfloat_t a_pos[VEC2_SIZE];
    mfloat_t b_pos[VEC2_SIZE];
    body_get_position(a, a_pos);
    body_get_position(b, b_pos);
    const float dist2 = vec2_distance_squared(a_pos, b_pos);
    return (GRAVITATIONAL_CONSTANT * a->mass * b->mass) / dist2;
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
    const sfVector2f mouse_w_pos = 
        sfRenderWindow_mapPixelToCoords(display->render_window, sfMouse_getPositionRenderWindow(display->render_window), display->view);
    const sfFloatRect bounds = sfCircleShape_getGlobalBounds(body->shape);
    if(sfFloatRect_contains(&bounds, mouse_w_pos.x, mouse_w_pos.y))
    {
        sfRenderWindow_drawText(display->render_window, body->info_text, NULL);
    }
}

Body* body_create(Display *display, Body *bodies, sfUint32 *num_of_bodies, float x, float y, sfUint32 mass)
{
    static sfUint32 last_body_id = 0;
    Body *body = NULL;
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape == NULL)
        {
            bodies[i].shape = sfCircleShape_create();
            bodies[i].info_text = sfText_create();
            body = &bodies[i];
            body->id = last_body_id;
            vec2_zero(body->vel);
            vec2_zero(body->acc);
            body_apply_mass(body, mass);
            last_body_id++;
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
        if(bodies[i].id == body->id)
        {
            sfCircleShape_destroy(bodies[i].shape);
            bodies[i].shape = NULL;
            sfText_destroy(bodies[i].info_text);
            bodies[i].info_text = NULL;
            (*num_of_bodies)--;
            break;
        }
    }
}