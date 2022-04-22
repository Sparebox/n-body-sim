#pragma once
#include <SFML/Graphics.h>

#define TITLE "n-body-sim"
#define FONT "res/fonts/F25_Bank_Printer.ttf"
#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080
#define WIN_CENTER_X WIN_WIDTH / 2
#define WIN_CENTER_Y WIN_HEIGHT / 2
#define MOUSE_PAN_SPEED 2
#define FPS 60

typedef struct {
    sfRenderWindow *render_window;
    sfView *view;
    sfView *gui_view;
    sfVideoMode mode;
    sfFont *font;
    sfVector2i last_mouse_pos;
    float zoom_level;
} Display;

void display_init(Display *display);
void display_handle_mouse_pan(Display *display);