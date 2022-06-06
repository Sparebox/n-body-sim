#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mathc.h"
#include "display.h"
#include "sim.h"
#include "editor.h"
#include "rot_body.h"

static void initialize(Sim *sim, const int argc, char *argv[]);
static void parse_arguments(Sim *sim, const int argc, char *argv[]);
static void render(Sim *sim);
static void update(Sim *sim);

static Rot_body body_a;
static Rot_body body_b;

int main(int argc, char *argv[])
{
    Sim *sim = calloc(1, sizeof(Sim));
    if(sim == NULL)
    {
        fprintf(stderr, "Could not allocate memory for sim struct\n");
        printf("Press ENTER to Continue\n");
        getchar();
        exit(EXIT_FAILURE);
    }
    initialize(sim, argc, argv);
    // Game loop
    while(sfRenderWindow_isOpen(sim->display.render_window))
    {
        update(sim);
        render(sim);
        sim->delta_time = sfClock_restart(sim->delta_clock);
    }
    // Release resources
    rot_body_destroy(&body_a);
    rot_body_destroy(&body_b);
    sim_destroy(sim);
    return EXIT_SUCCESS;
}

void parse_arguments(Sim *sim, const int argc, char *argv[])
{
    if(argc > 3)
    {
        fprintf(stderr, "Too many arguments given! Exiting\n");
        exit(EXIT_FAILURE);
    }
    if(argc > 1) // At least one argument given
    {
        if(!strcmp(argv[1], "-g"))
        {
            sim->sim_type = GRAVITATIONAL_SIM;
        }
        else if(!strcmp(argv[1], "-a"))
        {
            sim->sim_type = ATOMIC_FORCE_SIM;
        }
        else if(!strcmp(argv[1], "-r"))
        {
            sim->sim_type = ROTATIONAL_PHYSICS_SIM;
        }
        if(argc == 3) // Two arguments given
        {
            if(!strcmp(argv[2], "-b"))
            {
                sim->collision_type = BOUNCY_COLLISIONS;
            }
            else if(!strcmp(argv[2], "-m"))
            {
                sim->collision_type = MERGE_COLLISIONS;
            }
        }
        else
        {
            sim->collision_type = MERGE_COLLISIONS;
        }
    }
    else // No arguments given
    {
        sim->sim_type = GRAVITATIONAL_SIM;
        sim->collision_type = MERGE_COLLISIONS;
    }
    switch(sim->sim_type)
    {
        case GRAVITATIONAL_SIM:
            printf("Gravitational sim\n");
            break;
        case ATOMIC_FORCE_SIM:
            printf("Atomic force sim\n");
            break;
        case ROTATIONAL_PHYSICS_SIM:
            printf("Rotational physics sim\n");
            break;
    }
    switch(sim->collision_type)
    {
        case BOUNCY_COLLISIONS:
            printf("Bouncy collisions enabled\n");
            break;
        case MERGE_COLLISIONS:
            printf("Merge collisions enabled\n");
            break;
    }
}

void initialize(Sim *sim, const int argc, char *argv[]) 
{
    parse_arguments(sim, argc, argv);
    display_init(&sim->display);
    sim_init(sim);
    body_a = rot_body_create(&sim->display, WIN_CENTER_X, WIN_CENTER_Y, 100.f, 50.f);
    sim->editor.rot_body = &body_a;
    body_b = rot_body_create(&sim->display, WIN_CENTER_X, WIN_CENTER_Y + 50.f, 100.f, 50.f);
 }

void update(Sim *sim)
{
    sim_poll_events(sim);
    display_handle_mouse_pan(&sim->display, sim->editor_enabled);
    sim_update(sim);
    rot_body_update(&body_a, sfTime_asSeconds(sim->delta_time));
    rot_body_update(&body_b, sfTime_asSeconds(sim->delta_time));
    sim_sat_collision_resolution(&body_a, &body_b);
}

void render(Sim *sim)
{
    sfRenderWindow_clear(sim->display.render_window, sfBlack);
    // Start rendering
    sim_render(sim);
    rot_body_render(&sim->display, &body_a);
    rot_body_render(&sim->display, &body_b);
    // Stop rendering
    sfRenderWindow_display(sim->display.render_window);
}
