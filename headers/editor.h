#pragma once
#include "sim.h"
#include "display.h"

void editor_render(Sim *sim);
void editor_render_gui(Editor *editor, Display *display);
void editor_render_circle_tool(Sim *sim, sfVector2i mouse_pos);
void editor_init_gui(Editor *editor, Display *display);
void editor_apply_velocity(Editor *editor, Display *display);
void editor_destroy(Editor *editor);