#pragma once
#include <SFML/Graphics.h>

#define TITLE "n-body-sim"
#define FONT "res/fonts/F25_Bank_Printer.ttf"
#define WIN_WIDTH 1600
#define WIN_HEIGHT 900
#define WIN_CENTER_X WIN_WIDTH / 2
#define WIN_CENTER_Y WIN_HEIGHT / 2
#define FPS 60

typedef struct {
    sfRenderWindow *render_window;
    sfView *view;
    sfView *gui_view;
    sfVideoMode mode;
    sfFont *font;
    sfVector2i last_mouse_click_pos;
    sfBool mouse_was_on_body;
    float zoom_level;
} Display;

void display_init(Display *display);
void display_handle_mouse_pan(Display *display, sfBool editor_enabled);