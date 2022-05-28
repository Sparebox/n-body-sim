#include <stdlib.h>
#include <stdio.h>
#include "mathc.h"
#include "display.h"
#include "sim.h"
#include "editor.h"
#include "body.h"

static void initialize(Sim *sim);
static void render(Sim *sim);
static void update(Sim *sim);

int main()
{
    Sim *sim = calloc(1, sizeof(Sim));
    if(sim == NULL)
    {
        fprintf(stderr, "Could not allocate memory for sim struct\n");
        printf("Press ENTER to Continue\n");
        getchar();
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
 }

void update(Sim *sim)
{
    sim_poll_events(sim);
    display_handle_mouse_pan(&sim->display, sim->editor_enabled);
    sim_update(sim);
}

void render(Sim *sim)
{
    sfRenderWindow_clear(sim->display.render_window, sfBlack);
    // Start rendering
    sim_render(sim);
    // Stop rendering
    sfRenderWindow_display(sim->display.render_window);
}