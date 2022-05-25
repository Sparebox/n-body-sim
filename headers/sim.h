#pragma once
#include "display.h"
#include "body.h"
#define MAX_BODIES 4096
#define GRAVITATIONAL_CONSTANT 6.67e-1f
#define HAMAKER_COEFF 1e11f
#define EPSILON_0 1.0e-3f
#define GRAVITATIONAL_SIM 0
#define VELOCITY_LIMITED 1
#define BODY_SPLITTING_ENABLED 0
#define DISTANCE_THRESHOLD 3000.f // Maximum distance of gravitational force calculation

typedef struct {
    sfText *editor_text;
    sfText *delete_all_text;
    sfText *random_dist_text;
    sfText *create_circle_text;
    sfText *new_body_mass_text;
    Body *selected_body;
    sfVector2f selected_body_pos;
    sfCircleShape *tool_circle;
    sfCircleShape *body_preview_circle;
    sfBool circle_mode_enabled;
    sfUint32 new_body_mass;
} Editor;

typedef struct {
    sfClock *delta_clock;
    sfTime delta_time;
    sfText *fps_text;
    sfText *bodies_text;
    sfText *largest_mass_text;
    sfText *zoom_text;
    sfText *possible_collisions_text;
    sfText *paused_text;
    sfUint32 num_of_bodies;
    sfUint32 possible_collisions;
    sfUint32 sim_speed_multiplier;
    sfBool following_largest_body;
    sfBool following_selected_body;
    sfBool paused;
    sfBool editor_enabled;
    Display display;
    Editor editor;
    Body *largest_body;
    Body *followed_body;
    Body bodies[MAX_BODIES];
} Sim;

void sim_init_gui(Sim *sim);
void sim_update_gui(Sim *sim);
void sim_render_gui(Sim *sim);
void sim_update(Sim *sim);
void sim_render(Sim *sim);
void sim_destroy(Sim *sim);
void sim_create_circle(
    Sim *sim, 
    const float center_x, 
    const float center_y, 
    float radius, 
    sfUint32 count,
    sfUint32 mass,
    sfBool stationary);
void sim_create_grid(Sim *sim, sfUint32 count, float spacing);
void sim_create_line(Sim *sim, float x1, float y1, float x2, float y2, float spacing);
void sim_create_random_distribution(Sim *sim, sfUint32 count, sfBool stationary);
sfInt32 sim_random_int(sfInt32 min, sfInt32 max);
float sim_random_float(float min, float max);
void sim_random_vector(mfloat_t *result, sfUint32 min_length, sfUint32 max_length);
void sim_poll_events(Sim *sim);
void sim_handle_left_click(Sim *sim);
void sim_handle_mouse_scroll(Sim *sim, sfEvent *event);
sfText* sim_create_text(sfVector2f pos, sfVector2f scale, sfFont *font);
void sim_apply_gravitation_forces(Body *bodies);
void sim_gravitation_force(mfloat_t *result, Body *a, Body *b, float dist2);
void sim_apply_interatomic_forces(Body *bodies);
void sim_coulombic_force(mfloat_t *result, Body *a, Body *b, float dist2, mfloat_t *direction);
void sim_van_der_waals_force(mfloat_t *result, Body *a, Body *b, float dist2, mfloat_t *direction);