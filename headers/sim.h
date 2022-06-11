#pragma once
#include "display.h"
#include "body.h"
#include "rot_body.h"
#define MAX_BODIES 2048
#define SUB_STEPS 2
#define GRAVITATIONAL_CONSTANT 6.67e-1f
#define GRAVITY_ENABLED 1
#define HAMAKER_COEFF 1e11f // Van Der Waals forces
#define EPSILON_0 1.0e-3f   // Coulombic forces
#define RESTITUTION_COEFF 1.f
#define VELOCITY_LIMITED 1
#define BODY_SPEED_LIMIT 200.f // Pixels per second
#define DISTANCE_THRESHOLD 3000.f // Maximum distance in pixels of gravitational force calculation

typedef struct Editor {
    sfText *editor_text;
    sfText *delete_all_text;
    sfText *random_dist_text;
    sfText *create_circle_text;
    sfText *new_body_mass_text;
    sfText *torque_mode_text;
    sfText *torque_line_text;
    Body *selected_body;
    Rot_body *rot_body;
    sfVector2f selected_body_pos;
    sfCircleShape *tool_circle;
    sfCircleShape *body_preview_circle;
    sfBool circle_mode_enabled;
    sfBool torque_mode_enabled;
    sfUint32 new_body_mass;
} Editor;

typedef enum Sim_type {
    GRAVITATIONAL_SIM,      // 1# Cmd line argument flag -g
    ATOMIC_FORCE_SIM,       // 1# Cmd line argument flag -a
    ROTATIONAL_PHYSICS_SIM  // 1# Cmd line argument flag -r
} Sim_type;

typedef enum Collision_type {
    BOUNCY_COLLISIONS,      // 2# Cmd line argument flag -b
    MERGE_COLLISIONS        // 2# Cmd line argument flag -m
} Collision_type;

typedef enum Integrator {
    VERLET,                 // 3# Cmd line argument flag -v
    RUNGE_KUTTA             // 3# Cmd line argument flag -rk4
} Integrator;

typedef struct Sim {
    sfClock *delta_clock;
    sfTime delta_time;
    sfText *fps_text;
    sfText *bodies_text;
    sfText *largest_mass_text;
    sfText *zoom_text;
    sfText *possible_collisions_text;
    sfText *paused_text;
    sfUint32 num_of_bodies;
    sfUint32 num_of_possible_collisions;
    sfUint32 sim_speed_multiplier;
    sfBool following_largest_body;
    sfBool following_selected_body;
    sfBool paused;
    sfBool editor_enabled;
    sfThread *gui_update_thread;
    sfMutex *gui_mutex;
    Display display;
    Editor editor;
    Sim_type sim_type;
    Collision_type collision_type;
    Integrator integrator;
    Body *largest_body;
    Body *followed_body;
    Body bodies[MAX_BODIES];
} Sim;

void sim_init(Sim *sim);
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
    sfBool give_random_vel);
void sim_create_grid(Sim *sim, sfUint32 count, float spacing);
void sim_create_line(Sim *sim, float x1, float y1, float x2, float y2, float spacing);
void sim_create_random_distribution(Sim *sim, sfUint32 count, sfBool give_random_vel);
sfInt32 sim_random_int(sfInt32 min, sfInt32 max);
float sim_random_float(float min, float max);
void sim_random_vector(mfloat_t *result, sfUint32 min_length, sfUint32 max_length);
void sim_poll_events(Sim *sim);
sfText* sim_create_text(sfVector2f pos, sfVector2f scale, sfFont *font);
void sim_apply_gravitation_forces(Body *bodies);
void sim_gravitation_force(mfloat_t *result, Body *a, Body *b, float dist2);
void sim_apply_interatomic_forces(Body *bodies);
void sim_coulombic_force(mfloat_t *result, Body *a, Body *b, float dist2, mfloat_t *direction);
void sim_van_der_waals_force(mfloat_t *result, Body *a, Body *b, float dist2, mfloat_t *direction);
sfVector2f sim_to_sf_vector(mfloat_t *vec2);
void sim_from_sf_vector(mfloat_t *result, const sfVector2f sf_vec);
void sim_print_vector(const mfloat_t *vec);
void sim_print_sf_vector(const sfVector2f *vec);
sfVector2f sim_closest_point_to_line(const sfVector2f pos, const sfVector2f a, const sfVector2f b);
void sim_get_normal(mfloat_t *result, const mfloat_t *point_a, const mfloat_t *point_b);
void sim_sat_collision_resolution_rect_rect(Rot_body *a, Rot_body *b);
void sim_collision_resolution_circle_rect(Body *circle, Rot_body *rect);