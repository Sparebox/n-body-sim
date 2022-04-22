#include <stdlib.h>
#include <stdio.h>
#include "mathc.h"
#include "display.h"
#include "sim.h"

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
        sim->delta_time = sfClock_restart(sim->delta_clock);
        update(sim);
        render(sim);
    }
    // Release resources
    sim_destroy(sim);
    return EXIT_SUCCESS;
}

void initialize(Sim *sim) 
{
    sim->delta_clock = sfClock_create();
    display_init(&sim->display);
    sim_init_gui(sim);
    sim_create_random_distribution(sim, 1000);
    // Body *a = body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X, WIN_CENTER_Y - 50, 500);
    // mfloat_t init_vel[] = {BODY_SPEED_LIMIT, 0};
    // vec2_assign(a->vel, init_vel);
    // Body *b = body_create(&sim->display, sim->bodies, &sim->num_of_bodies, WIN_CENTER_X + 200, WIN_CENTER_Y, 1000);
    //sim_create_circle(sim, WIN_CENTER_X, WIN_CENTER_Y, 100, 60);
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
            body_update(&sim->display, &sim->bodies[i], sim->bodies, &sim->num_of_bodies, sfTime_asSeconds(sim->delta_time));
        }
    }
    char fps_string[16];
    sprintf(fps_string, "FPS %.1f", 1 / sfTime_asSeconds(sim->delta_time));
    sfText_setString(sim->fps_text, fps_string);
    char bodies_string[32];
    sprintf(bodies_string, "BODIES %d", sim->num_of_bodies);
    sfText_setString(sim->bodies_text, bodies_string);
    char mass_string[32];
    sprintf(mass_string, "LARGEST MASS %d", sim->largest_body->mass);
    sfText_setString(sim->largest_mass_text, mass_string);
}

void render(Sim *sim)
{
    sfRenderWindow_clear(sim->display.render_window, sfBlack);
    // Start rendering
    if(sim->following_largest_body)
    {
        if(sim->largest_body->shape != NULL)
        {
            const sfVector2f view_center = sfCircleShape_getPosition(sim->largest_body->shape);
            sfView_setCenter(sim->display.view, view_center);
        }
    }
    sfRenderWindow_setView(sim->display.render_window, sim->display.view);
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