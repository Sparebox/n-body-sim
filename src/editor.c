#include <stdio.h>
#include "editor.h"

void editor_init_gui(Editor *editor, Display *display)
{
    editor->editor_text = sfText_create();
    sfText_setString(editor->editor_text, "EDITOR");
    sfText_setFont(editor->editor_text, display->font);
    sfText_setColor(editor->editor_text, sfWhite);
    sfFloatRect bounds = sfText_getGlobalBounds(editor->editor_text);
    sfVector2f scale = {1.f, 1.f};
    sfVector2f pos = {WIN_CENTER_X - bounds.width / 2.f, 10.f};
    sfText_setPosition(editor->editor_text, pos);
    sfText_setScale(editor->editor_text, scale);

    editor->delete_all_text = sfText_create();
    sfText_setString(editor->delete_all_text, "[X] DELETE ALL");
    sfText_setFont(editor->delete_all_text, display->font);
    sfText_setColor(editor->delete_all_text, sfWhite);
    scale.x = 0.5f;
    scale.y = 0.5f;
    sfText_setScale(editor->delete_all_text, scale);
    bounds = sfText_getGlobalBounds(editor->delete_all_text);
    pos.x = WIN_WIDTH - bounds.width - 70.f;
    pos.y = 0.f;
    sfText_setPosition(editor->delete_all_text, pos);

    editor->random_dist_text = sfText_create();
    sfText_setString(editor->random_dist_text, "[R] ADD 1000 BODIES");
    sfText_setFont(editor->random_dist_text, display->font);
    sfText_setColor(editor->random_dist_text, sfWhite);
    sfText_setScale(editor->random_dist_text, scale);
    bounds = sfText_getGlobalBounds(editor->random_dist_text);
    pos.y += 15.f;
    sfText_setPosition(editor->random_dist_text, pos);

    editor->create_circle_text = sfText_create();
    sfText_setString(editor->create_circle_text, "[C] CREATE CIRCLE");
    sfText_setFont(editor->create_circle_text, display->font);
    sfText_setColor(editor->create_circle_text, sfWhite);
    sfText_setScale(editor->create_circle_text, scale);
    bounds = sfText_getGlobalBounds(editor->create_circle_text);
    pos.y += 15.f;
    sfText_setPosition(editor->create_circle_text, pos);

    editor->new_body_mass_text = sfText_create();
    sfText_setString(editor->new_body_mass_text, "MASS");
    sfText_setFont(editor->new_body_mass_text, display->font);
    sfText_setColor(editor->new_body_mass_text, sfRed);
    sfText_setScale(editor->new_body_mass_text, scale);

    editor->body_preview_circle = sfCircleShape_create();
    sfCircleShape_setFillColor(editor->body_preview_circle, sfTransparent);
    sfCircleShape_setOutlineColor(editor->body_preview_circle, sfWhite);
    sfCircleShape_setOutlineThickness(editor->body_preview_circle, CIRCLE_TOOL_LINE_THICKNESS);
    
}


void editor_render(Sim *sim)
{
    const sfVector2i mouse_pos = sfMouse_getPositionRenderWindow(sim->display.render_window);
    // Editor velocity line
    if(sfMouse_isButtonPressed(sfMouseLeft) &&
    !sim->editor.circle_mode_enabled &&
    sim->editor.selected_body_pos.x != 0.f &&
    sim->editor.selected_body_pos.y != 0.f &&
    sim->editor.selected_body != NULL)
    {
        sfVertex vertex1 = {
            .color = sfWhite,
            .position = sim->editor.selected_body_pos};
        sfVertex vertex2 = {
            .color = sfWhite, 
            .position = sfRenderWindow_mapPixelToCoords(sim->display.render_window, mouse_pos, sim->display.view)
        };
        sfVertex vel_line[] = {vertex1, vertex2};
        sfRenderWindow_drawPrimitives(sim->display.render_window, vel_line, 2, sfLines, NULL);
    }
    // Circle tool drawing
    editor_render_circle_tool(sim, mouse_pos);
    // Circle tool mass text
    if(!sim->editor.circle_mode_enabled)
    {
        sfRenderWindow_drawCircleShape(sim->display.render_window, sim->editor.body_preview_circle, NULL); 
        sfRenderWindow_drawText(sim->display.render_window, sim->editor.new_body_mass_text, NULL);
    }
}

void editor_update(Sim *sim)
{
    const sfVector2i mouse_pos = sfMouse_getPositionRenderWindow(sim->display.render_window);
    const sfVector2f world_mouse_pos = sfRenderWindow_mapPixelToCoords(sim->display.render_window, mouse_pos, sim->display.view);
    // Update body preview
    sfCircleShape_setRadius(sim->editor.body_preview_circle, sim->editor.new_body_mass / BODY_RADIUS_FACTOR);
    const sfVector2f new_preview_pos = 
        {
            world_mouse_pos.x - sim->editor.new_body_mass / BODY_RADIUS_FACTOR,
            world_mouse_pos.y - sim->editor.new_body_mass / BODY_RADIUS_FACTOR
        };
    sfCircleShape_setPosition(
        sim->editor.body_preview_circle,
        new_preview_pos
    );
    // New body mass text
    if(!sim->editor.circle_mode_enabled)
    {
        const sfVector2f text_scale = {sim->display.zoom_level, sim->display.zoom_level};
        sfText_setPosition(sim->editor.new_body_mass_text, new_preview_pos);
        sfText_setScale(sim->editor.new_body_mass_text, text_scale);
        const sfFloatRect text_bounds = sfText_getGlobalBounds(sim->editor.new_body_mass_text);
        sfVector2f new_pos;
        new_pos.x = new_preview_pos.x + sim->editor.new_body_mass / BODY_RADIUS_FACTOR - text_bounds.width / 2;
        new_pos.y = new_preview_pos.y + sim->editor.new_body_mass / BODY_RADIUS_FACTOR;
        sfText_setPosition(sim->editor.new_body_mass_text, new_pos);
    }
}

void editor_render_gui(Editor *editor, Display *display)
{
    sfRenderWindow_setView(display->render_window, display->gui_view);
    sfRenderWindow_drawText(display->render_window, editor->editor_text, NULL);
    sfRenderWindow_drawText(display->render_window, editor->delete_all_text, NULL);
    sfRenderWindow_drawText(display->render_window, editor->random_dist_text, NULL);
    sfRenderWindow_drawText(display->render_window, editor->create_circle_text, NULL);
}

void editor_render_circle_tool(Sim *sim, sfVector2i mouse_pos)
{
    sfText_setColor(sim->editor.create_circle_text, sim->editor.circle_mode_enabled ? sfRed : sfWhite);
    if(sfMouse_isButtonPressed(sfMouseLeft) && 
    sim->editor.circle_mode_enabled &&
    sim->editor.tool_circle != NULL &&
    sim->display.last_mouse_click_pos.x != 0.f &&
    sim->display.last_mouse_click_pos.y != 0.f)
    {
        const sfVector2f world_last_mouse_click_pos =
            sfRenderWindow_mapPixelToCoords(sim->display.render_window, sim->display.last_mouse_click_pos, sim->display.view);
        const sfVector2f world_current_mouse_pos =
            sfRenderWindow_mapPixelToCoords(sim->display.render_window, mouse_pos, sim->display.view);
        mfloat_t circle_center[VEC2_SIZE];
        mfloat_t mouse_pos_vec[VEC2_SIZE];
        circle_center[0] = world_last_mouse_click_pos.x;
        circle_center[1] = world_last_mouse_click_pos.y;
        mouse_pos_vec[0] = world_current_mouse_pos.x;
        mouse_pos_vec[1] = world_current_mouse_pos.y;
        const float circle_radius = vec2_distance(circle_center, mouse_pos_vec);
        sfCircleShape_setRadius(sim->editor.tool_circle, circle_radius);
        const sfVector2f new_pos = {world_last_mouse_click_pos.x - circle_radius, world_last_mouse_click_pos.y - circle_radius};
        sfCircleShape_setPosition(sim->editor.tool_circle, new_pos);
        sfRenderWindow_drawCircleShape(sim->display.render_window, sim->editor.tool_circle, NULL);
    }
}

void editor_apply_velocity(Editor *editor, Display *display) 
{
    const sfVector2i mouse_pos = sfMouse_getPositionRenderWindow(display->render_window);
    const sfVector2i selected_body_screen_pos = 
        sfRenderWindow_mapCoordsToPixel(display->render_window, editor->selected_body_pos, display->view);
    editor->selected_body->vel[0] += selected_body_screen_pos.x - mouse_pos.x;
    editor->selected_body->vel[1] += selected_body_screen_pos.y - mouse_pos.y;
}

void editor_destroy(Editor *editor)
{
    sfText_destroy(editor->editor_text);
    sfText_destroy(editor->delete_all_text);
    sfText_destroy(editor->random_dist_text);
    sfText_destroy(editor->create_circle_text);
    sfText_destroy(editor->new_body_mass_text);
    if(editor->tool_circle != NULL)
    {
        sfCircleShape_destroy(editor->tool_circle);
    }
    sfCircleShape_destroy(editor->body_preview_circle);
}