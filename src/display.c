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
    display_setview(display, 0, 0, WIN_WIDTH, WIN_HEIGHT);
    display->font = sfFont_createFromFile(FONT);
    if(display->font == NULL) 
    {
        fprintf(stderr, "Error loading font: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

inline void display_setview(Display *display, float x1, float y1, float x2, float y2) 
{
    sfFloatRect view_rectangle = {x1, y1, x2, y2};
    display->view = sfView_createFromRect(view_rectangle);
    sfRenderWindow_setView(display->render_window, display->view);
}
