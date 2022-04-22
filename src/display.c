#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "mathc.h"
#include "display.h"

void display_init(Display *display) 
{
    sfVideoMode mode = {WIN_WIDTH, WIN_HEIGHT, 32};
    display->mode = mode;
    display->render_window = sfRenderWindow_create(display->mode, TITLE, sfClose, NULL);
    sfRenderWindow_setFramerateLimit(display->render_window, FPS);
    sfFloatRect view_rectangle = {0, 0, WIN_WIDTH, WIN_HEIGHT};
    display->view = sfView_createFromRect(view_rectangle);
    display->gui_view = sfView_createFromRect(view_rectangle);
    display->font = sfFont_createFromFile(FONT);
    if(display->font == NULL) 
    {
        fprintf(stderr, "Error loading font: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void display_handle_mouse_pan(Display *display) 
{
    if(!sfMouse_isButtonPressed(sfMouseLeft))
    {   
        return;
    }
    mfloat_t mouse_pos[VEC2_SIZE];
    mfloat_t last_mouse_pos[VEC2_SIZE];
    mfloat_t diff[VEC2_SIZE];
    const sfVector2i pos = sfMouse_getPositionRenderWindow(display->render_window);
    mouse_pos[0] = pos.x;
    mouse_pos[1] = pos.y;
    last_mouse_pos[0] = display->last_mouse_pos.x;
    last_mouse_pos[1] = display->last_mouse_pos.y;
    display->last_mouse_pos = pos;
    vec2_subtract(diff, last_mouse_pos, mouse_pos);
    vec2_multiply_f(diff, diff, MOUSE_PAN_SPEED);
    const sfVector2f offset = {diff[0], diff[1]};
    sfView_move(display->view, offset);
}
