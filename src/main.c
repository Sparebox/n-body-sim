#include <stdlib.h>
#include <stdio.h>
#include "mathc.h"
#include "display.h"
#include "sim.h"

#define NUM_OF_BODIES 100
 
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

inline void initialize(Sim *sim) 
{
    sim->delta_clock = sfClock_create();
    display_init(&sim->display);
    sim_create_random_distribution(sim, 1000);
    sim_create_circle(sim, WIN_CENTER_X, WIN_CENTER_Y, 300, 100);
}

void update(Sim *sim)
{
    sim_poll_events(sim);
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL)
        {
            body_update(&sim->bodies[i], sim->bodies, sfTime_asSeconds(sim->delta_time));
        }
    }
}

void render(Sim *sim)
{
    sfRenderWindow_clear(sim->display.render_window, sfBlack);
    // Start rendering
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL)
        {
            body_render(&sim->display, &sim->bodies[i]);  
        }
    }
    // Stop rendering
    sfRenderWindow_display(sim->display.render_window);
}