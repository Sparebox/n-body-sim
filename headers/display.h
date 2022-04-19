#pragma once
#include <SFML/Graphics.h>

#define TITLE "n-body-sim"
#define FONT "res/fonts/F25_Bank_Printer.ttf"
#define WIN_WIDTH 1920
#define WIN_HEIGHT 1080
#define WIN_CENTER_X WIN_WIDTH / 2
#define WIN_CENTER_Y WIN_HEIGHT / 2
#define FPS 60

typedef struct {
    sfRenderWindow *render_window;
    sfView *view;
    sfVideoMode mode;
    sfFont *font;
} Display;

void display_init(Display *display);
void display_setview(Display *display, float x1, float y1, float x2, float y2);
