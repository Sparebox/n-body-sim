#include "rot_body.h"
#include "display.h"
#include "sim.h"
#include <assert.h>
#include <stdio.h>

static void verlet_integrate(Rot_body *body, mfloat_t *new_pos, float *new_angle, const float delta_time);
static void calculate_moment_of_inertia(Rot_body *body);

void rot_body_update(Rot_body *body, const float delta_time)
{
    float new_angle = 0.f;
    mfloat_t new_pos[VEC2_SIZE];
    verlet_integrate(body, new_pos, &new_angle, delta_time);
    sfRectangleShape_rotate(body->shape, new_angle);
    sfRectangleShape_setPosition(body->shape, sim_to_sf_vector(new_pos));
}

void rot_body_render(Display *display, Rot_body *body)
{
    const sfVector2i screen_pos = 
        sfRenderWindow_mapCoordsToPixel(display->render_window, sfRectangleShape_getPosition(body->shape), display->view);
    const sfIntRect viewport = {0, 0, WIN_WIDTH, WIN_HEIGHT};
    if(!sfIntRect_contains(&viewport, screen_pos.x, screen_pos.y)) // If body is not in window don't render
    {
        return;
    }
    sfRenderWindow_setView(display->render_window, display->view);
    sfRenderWindow_drawRectangleShape(display->render_window, body->shape, NULL);

    sfFloatRect text_bounds = { 0 };
    sfVector2f offset = { 0 };
    const sfVector2f text_scale = {display->zoom_level, display->zoom_level};
    const sfVector2f body_pos = sfRectangleShape_getPosition(body->shape);
    const sfVector2f mouse_w_pos = 
        sfRenderWindow_mapPixelToCoords(display->render_window, sfMouse_getPositionRenderWindow(display->render_window), display->view);
    const sfFloatRect bounds = sfRectangleShape_getGlobalBounds(body->shape);
    // Show info text if mouse is on body
    if(sfFloatRect_contains(&bounds, mouse_w_pos.x, mouse_w_pos.y))
    {
        sfText_setScale(body->info_text, text_scale);
        text_bounds = sfText_getGlobalBounds(body->info_text);
        offset.x = body_pos.x - text_bounds.width / 2;
        offset.y = body_pos.y;
        sfText_setPosition(body->info_text, offset);
        sfRenderWindow_drawText(display->render_window, body->info_text, NULL);
    }
}

Rot_body rot_body_create(Display *display, const float x, const float y, const float width, const float height)
{
    Rot_body body = { 0 };
    body.shape = sfRectangleShape_create();
    if(body.shape == NULL)
    {
        fprintf(stderr, "Could not create rotational body!\n");
        return (Rot_body) { 0 };
    }
    vec2_zero(body.acc);
    vec2_zero(body.vel);
    vec2_zero(body.last_acc);
    body.angular_vel = 0.f;
    body.angular_acc = 0.f;
    body.mass = width * height * ROT_BODY_DENSITY;
    assert(body.mass > 0.f);
    const sfVector2f pos = {x, y};
    const sfVector2f origin = {width / 2.f, height / 2.f};
    const sfVector2f scale = {width, height};
    sfRectangleShape_setSize(body.shape, scale);
    sfRectangleShape_setOrigin(body.shape, origin);
    sfRectangleShape_setPosition(body.shape, pos);
    calculate_moment_of_inertia(&body);
    const sfUint8 red = sim_random_int(0, 255);
    const sfUint8 green = sim_random_int(0, 255);
    const sfUint8 blue = sim_random_int(0, 255);
    const sfColor color = sfColor_fromRGB(red, green, blue);
    sfRectangleShape_setFillColor(body.shape, color);

    body.info_text = sfText_create();
    sfText_setPosition(body.info_text, pos);
    sfText_setOrigin(body.info_text, origin);
    sfText_setFont(body.info_text, display->font);
    sfText_setColor(body.info_text, sfRed);
    char info_string[32] = { 0 };
    sprintf(info_string, "Moment of inertia: %.2f", body.moment_of_inertia);
    sfText_setString(body.info_text, info_string);

    return body;
}

void rot_body_destroy(Rot_body *body)
{
    sfRectangleShape_destroy(body->shape);
    sfText_destroy(body->info_text);
}

void rot_body_apply_torque(Rot_body *body, const float torque)
{
    assert(body->moment_of_inertia > 0.f);
    body->angular_acc += torque / body->moment_of_inertia;
}

void rot_body_apply_force(Rot_body *body, mfloat_t *force)
{
    assert(body->mass > 0.f);
    vec2_divide_f(force, force, body->mass);
    vec2_add(body->acc, body->acc, force);
}

// Moment of inertia for a rectangular shape
void calculate_moment_of_inertia(Rot_body *body)
{
    const sfVector2f size = sfRectangleShape_getSize(body->shape);
    const float width = size.x;
    const float height = size.y;
    body->moment_of_inertia = (1.f / 12.f) * body->mass * (powf(width, 2.f) + powf(height, 2.f));
    assert(body->moment_of_inertia > 0.f);
}

void verlet_integrate(Rot_body *body, mfloat_t *new_pos, float *new_angle, const float delta_time)
{
    mfloat_t pos[VEC2_SIZE];
    mfloat_t temp[VEC2_SIZE];
    sim_from_sf_vector(pos, sfRectangleShape_getPosition(body->shape));
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
    // New angle
    float angle = body->angular_vel * delta_time;
    angle += 0.5f * body->angular_acc * delta_time * delta_time;
    (*new_angle) += angle;
    // New angular velocity
    float angular_vel = (body->last_angular_acc + body->angular_acc) * 0.5f;
    angular_vel *= delta_time;
    body->angular_vel += angular_vel;
    // Apply rotational friction
    body->angular_vel *= ROTATION_FRICTION_COEFF;
    body->last_angular_acc = body->angular_acc;
    body->angular_acc = 0.f;
}

