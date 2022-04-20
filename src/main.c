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
    display_init(&sim->display);

    sim->fps_text = sfText_create();
    sfText_setFont(sim->fps_text, sim->display.font);
    sfText_setColor(sim->fps_text, sfRed);
    const sfVector2f scale = {0.5f, 0.5f};
    const sfVector2f pos = {0, 0};
    sfText_setPosition(sim->fps_text, pos);
    sfText_setScale(sim->fps_text, scale);

    sim_create_random_distribution(sim, 1500);
}

void update(Sim *sim)
{
    sim_poll_events(sim);
    for(size_t i = 0; i < MAX_BODIES; i++)
    {
        if(sim->bodies[i].shape != NULL)
        {
            body_update(&sim->display, &sim->bodies[i], sim->bodies, sfTime_asSeconds(sim->delta_time));
        }
    }
    char fps_string[16];
    sprintf(fps_string, "FPS %.2f", 1 / sfTime_asSeconds(sim->delta_time));
    sfText_setString(sim->fps_text, fps_string);
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
    sfRenderWindow_drawText(sim->display.render_window, sim->fps_text, NULL);
    // Stop rendering
    sfRenderWindow_display(sim->display.render_window);
}