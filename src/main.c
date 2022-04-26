#include <stdlib.h>
#include <stdio.h>
#include "mathc.h"
#include "display.h"
#include "sim.h"
#include "body.h"

void initialize(Sim *sim);
void render(Sim *sim);
void update(Sim *sim);

int main()
{
    Sim *sim = calloc(1, sizeof(Sim));
    if(sim == NULL)
    {
        fprintf(stderr, "Could not allocate memory for sim struct\n");
        exit(EXIT_FAILURE);
    }
    initialize(sim);
    // Game loop
    while(sfRenderWindow_isOpen(sim->display.render_window))
    {
        update(sim);
        render(sim);
        sim->delta_time = sfClock_restart(sim->delta_clock);
    }
    // Release resources
    sim_destroy(sim);
    return EXIT_SUCCESS;
}

void initialize(Sim *sim) 
{
    sim->delta_clock = sfClock_create();
    sim->sim_speed_multiplier = 1;
    display_init(&sim->display);
    sim_init_gui(sim);
    sim_create_random_distribution(sim, 1000, sfTrue);
    //body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X, WIN_CENTER_Y, 500);
    // mfloat_t init_vel[] = {200.f, 0.f};
    // vec2_assign(a->vel, init_vel);
    // body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X, WIN_CENTER_Y, 1000);
    // body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X - 10, WIN_CENTER_Y - 50, 1000);
    // body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X + 50, WIN_CENTER_Y - 100, 1000);
    // body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X + 50, WIN_CENTER_Y - 130, 1000);
    // body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X + 50, WIN_CENTER_Y + 100, 1000);
    // body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X + 50, WIN_CENTER_Y - 150, 1000);
    // body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X + 100, WIN_CENTER_Y - 150, 1000);
    // init_vel[0] = -40.f;
    // init_vel[1] = -40.f;
    // vec2_assign(b->vel, init_vel);
    //sim_create_circle(sim, WIN_CENTER_X, WIN_CENTER_Y, 50, 10, 1000, sfTrue);
    // Body *possible_collisions[MAX_BODIES] = { 0 };
    // body_sweep_and_prune(sim->bodies, possible_collisions);
    // sfUint32 possibilities = 0;
    // for(size_t i = 0; i < MAX_BODIES; i++)
    // {
    //     if(possible_collisions[i] != NULL)
    //     {
    //         possibilities++;
    //     }
    // }
    // printf("Possible colliding bodies: %d\n", possibilities);
}

void update(Sim *sim)
{
    sim_poll_events(sim);
    display_handle_mouse_pan(&sim->display);
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
                &sim->display,
                &sim->bodies[i],
                sim->bodies,
                &sim->num_of_bodies,
                sfTime_asSeconds(sim->delta_time),
                sim->sim_speed_multiplier
            );
        }
    }
    Body *possible_collisions[MAX_BODIES] = { 0 };
    body_sweep_and_prune(sim->bodies, possible_collisions);
    body_check_collisions(&sim->display, possible_collisions, sim->bodies, &sim->num_of_bodies);
    sim_update_gui(sim);
}

void render(Sim *sim)
{
    sfRenderWindow_clear(sim->display.render_window, sfBlack);
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
    // Start rendering
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL)
        {
            body_render(&sim->display, &sim->bodies[i]);
        }
    }
    sim_render_gui(sim);
    // Stop rendering
    sfRenderWindow_display(sim->display.render_window);
}