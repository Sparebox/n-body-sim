#include <stdio.h>
#include "body.h"
#include "sim.h"
#include "SFML/Graphics.h"

const mfloat_t g_GRAVITY[] = {0, 9.81f};

void body_update(Body *body, Body *bodies, float delta_time)
{
    // Gravity
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].id != body->id && bodies[i].shape != NULL)
        {
            const float force = body_calculate_gravitation_force(body, &bodies[i]);
            mfloat_t pos_a[VEC2_SIZE];
            mfloat_t pos_b[VEC2_SIZE];
            body_get_position(body, pos_a);
            body_get_position(&bodies[i], pos_b);
            float radius_a = sfCircleShape_getRadius(body->shape);
            float radius_b = sfCircleShape_getRadius(bodies[i].shape);
            if(vec2_distance(pos_a, pos_b) < radius_a + radius_b)
            {
                body_handle_collision(body, &bodies[i], bodies);
                return;
            }
            mfloat_t dir[VEC2_SIZE];
            vec2_subtract(dir, pos_b, pos_a);
            vec2_normalize(dir, dir);
            vec2_multiply_f(dir, dir, force);
            body_apply_force(body, dir[0], dir[1]);
        }
    }
    //
    vec2_multiply_f(body->acc, body->acc, delta_time);
    vec2_add(body->vel, body->vel, body->acc);
    // Limit speed
    if(vec2_length(body->vel) > BODY_SPEED_LIMIT)
    {
        vec2_normalize(body->vel, body->vel);
        vec2_multiply_f(body->vel, body->vel, BODY_SPEED_LIMIT);
    }
    vec2_zero(body->acc);
    const sfVector2f offset = {body->vel[0], body->vel[1]};
    sfCircleShape_move(body->shape, offset);
    sfText_move(body->info_text, offset);

    const sfVector2f pos = sfCircleShape_getPosition(body->shape);
    sfVector2f new_pos = pos;
    if(pos.x > WIN_WIDTH)
    {
        new_pos.x = 0;
    }
    else if(pos.x < 0)
    {
        new_pos.x = WIN_WIDTH;
    }
    if(pos.y > WIN_HEIGHT)
    {
        new_pos.y = 0;
    } 
    else if(pos.y < 0)
    {
        new_pos.y = WIN_HEIGHT;
    }
    if(new_pos.x != pos.x && new_pos.y != pos.y)
    {
        sfCircleShape_setPosition(body->shape, new_pos);
    }
}

void body_handle_collision(Body *a, Body *b, Body *bodies)
{
    mfloat_t a_pos[VEC2_SIZE];
    mfloat_t b_pos[VEC2_SIZE];
    mfloat_t mv_a[VEC2_SIZE];
    mfloat_t mv_b[VEC2_SIZE];
    vec2_assign(mv_a, a->vel);
    vec2_assign(mv_b, b->vel);
    vec2_multiply_f(mv_a, mv_a, a->mass);
    vec2_multiply_f(mv_b, mv_b, b->mass);
    mfloat_t new_vel[VEC2_SIZE];
    vec2_add(new_vel, mv_a, mv_b);
    vec2_divide_f(new_vel, new_vel, a->mass + b->mass);
    body_get_position(a, a_pos);
    body_get_position(b, b_pos);
    Body *survivor = a->mass > b->mass ? a : b;
    vec2_assign(survivor->vel, new_vel);
    body_apply_mass(survivor, a->mass + b->mass);
    body_destroy(survivor == a ? b : a, bodies);
}

void body_apply_force(Body *body, float x, float y) 
{   
    mfloat_t force[] = {x, y};
    vec2_divide_f(force, force, body->mass);
    vec2_add(body->acc, body->acc, force);
}

void body_apply_mass(Body *body, float mass)
{
    body->mass = mass;
    const float radius = mass / 15;
    sfCircleShape_setRadius(body->shape, radius);
    const sfVector2f origin = {radius, radius};
    sfCircleShape_setOrigin(body->shape, origin);
    char info_string[32];
    sprintf(info_string, "Mass: %.2f", mass);
    sfText_setString(body->info_text, info_string);
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
    // const sfVector2f pos = sfCircleShape_getPosition(body->shape);
    // if(pos.x > WIN_WIDTH || pos.x < 0 || pos.y > WIN_HEIGHT || pos.y < 0) // Body is out of window
    // {
    //     return;
    // }
    sfRenderWindow_drawCircleShape(display->render_window, body->shape, NULL);
    const sfVector2i mouse_pos = sfMouse_getPosition(display->render_window);
    const sfFloatRect bounds = sfCircleShape_getGlobalBounds(body->shape);
    if(sfFloatRect_contains(&bounds, mouse_pos.x, mouse_pos.y))
    {
        sfRenderWindow_drawText(display->render_window, body->info_text, NULL);
    }
    //sfRenderWindow_drawCircleShape(display->render_window, body->debug_shape, NULL);
}

Body* body_create(Display *display, Body *bodies, float x, float y, float mass)
{
    static sfUint32 last_id = 0;
    //sfCircleShape *shape = NULL;
    Body *body = NULL;
    //sfCircleShape *debug_shape = NULL;
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].shape == NULL)
        {
            bodies[i].shape = sfCircleShape_create();
            bodies[i].info_text = sfText_create();
            body = &bodies[i];
            body->id = last_id;
            vec2_zero(body->vel);
            vec2_zero(body->acc);
            body_apply_mass(body, mass);
            last_id++;
            break;
        }
    }
    if(body == NULL)
    {
        printf("Body array is full!\n");
        return NULL;
    }
    const sfVector2f pos = {x, y};
    const sfVector2f origin = {mass / 2, mass / 2};
    sfCircleShape_setOrigin(body->shape, origin);
    sfCircleShape_setPosition(body->shape, pos);
    sfCircleShape_setFillColor(body->shape, sfWhite);
    sfText_setPosition(body->info_text, pos);
    sfText_setOrigin(body->info_text, origin);
    sfText_setFont(body->info_text, display->font);
    sfText_setColor(body->info_text, sfRed);
    char info_string[32] = { 0 };
    sprintf(info_string, "Mass: .2%f", mass);
    sfText_setString(body->info_text, info_string);
    return body;
    // Debug
    // const sfVector2f debug_pos = {x - 1, y - 1};
    // sfCircleShape_setPosition(debug_shape, debug_pos);
    // sfCircleShape_setFillColor(debug_shape, sfRed);
    // sfCircleShape_setRadius(debug_shape, 2);
}

void body_destroy(Body *body, Body *bodies)
{
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(bodies[i].id == body->id)
        {
            sfCircleShape_destroy(bodies[i].shape);
            bodies[i].shape = NULL;
            sfText_destroy(bodies[i].info_text);
            bodies[i].info_text = NULL;
            printf("Body destroyed!\n");
            break;
        }
    }
}