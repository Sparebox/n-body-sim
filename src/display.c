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
        perror("Error loading font");
        printf("Press ENTER to Continue\n");
        getchar();
        exit(EXIT_FAILURE);
    }
    display->zoom_level = 1.f;
}

void display_handle_mouse_pan(Display *display, sfBool editor_enabled) 
{
    if((!editor_enabled && !sfMouse_isButtonPressed(sfMouseLeft)) || display->mouse_was_on_body)
    {   
        return;
    }
    else if(editor_enabled && !sfMouse_isButtonPressed(sfMouseRight))
    {
        return;
    }
    mfloat_t mouse_pos[VEC2_SIZE];
    mfloat_t last_mouse_click_pos[VEC2_SIZE];
    mfloat_t diff[VEC2_SIZE];
    const sfVector2i pos = sfMouse_getPositionRenderWindow(display->render_window);
    mouse_pos[0] = pos.x;
    mouse_pos[1] = pos.y;
    last_mouse_click_pos[0] = display->last_mouse_click_pos.x;
    last_mouse_click_pos[1] = display->last_mouse_click_pos.y;
    display->last_mouse_click_pos = pos;
    vec2_subtract(diff, last_mouse_click_pos, mouse_pos);
    vec2_multiply_f(diff, diff, display->zoom_level);
    const sfVector2f offset = {diff[0], diff[1]};
    sfView_move(display->view, offset);
}

void display_destroy(Display *display)
{
    sfView_destroy(display->view);
    sfView_destroy(display->gui_view);
    sfFont_destroy(display->font);
    sfRenderWindow_destroy(display->render_window);
}
