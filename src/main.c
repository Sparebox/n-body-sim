#include <stdlib.h>
#include <stdio.h>
#include "mathc.h"
#include "display.h"
#include "sim.h"
#include "editor.h"
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
    display_init(&sim->display);
    sim_init_gui(sim);
    editor_init_gui(&sim->editor, &sim->display);
 }

void update(Sim *sim)
{
    sim_poll_events(sim);
    display_handle_mouse_pan(&sim->display, sim->editor_enabled);
    if(!sim->editor_enabled)
    {
        sim_update(sim);
    }
    sim_update_gui(sim);
}

void render(Sim *sim)
{
    sfRenderWindow_clear(sim->display.render_window, sfBlack);
    // Start rendering
    sim_render(sim);
    if(sim->editor_enabled)
    {
        editor_render_gui(&sim->editor, &sim->display);
    }
    sim_render_gui(sim); 
    // Stop rendering
    sfRenderWindow_display(sim->display.render_window);
}