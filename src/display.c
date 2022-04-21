#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
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
