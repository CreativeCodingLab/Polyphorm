#pragma once
#include "maths.h"

enum ShadingType {
    SOLID_COLOR,
    LINES
};

struct Font;

struct Panel {
    char *name;
    Vector2 pos;
    float width;

    Vector2 item_pos;
};

namespace ui {
    void init(float screen_width, float screen_height);

    void draw_text(const char *text, Font *font, float x, float y, Vector4 color, Vector2 origin = Vector2(0,0));
    void draw_text(const char *text, Font *font, Vector2 pos, Vector4 color, Vector2 origin = Vector2(0,0));
    void draw_text(const char *text, Vector2 pos, Vector4 color, Vector2 origin = Vector2(0,0));

    void draw_rect(float x, float y, float width, float height, Vector4 color, ShadingType shading_type = SOLID_COLOR);
    void draw_rect(Vector2 pos, float width, float height, Vector4 color, ShadingType shading_type = SOLID_COLOR);

    void draw_triangle(Vector2 v1, Vector2 v2, Vector2 v3, Vector4 color);
    void draw_line(Vector2 *points, int point_count, float width, Vector4 color);

    //NEW ADDED IN CPPLIB UPDATE
    void draw_arc(Vector2 pos, float radius_min, float radius_max, float start_radian, float end_radian, Vector4 color);
    void draw_circle(Vector2 pos, float radius, Vector4 color);

    Panel start_panel(char *name, Vector2 pos);
    Panel start_panel(char *name, float x, float y);
    Panel start_panel(char *name, Vector2 pos, float width);
    Panel start_panel(char *name, float x, float y, float width);
    void end_panel(Panel *panel);
    Vector4 get_panel_rect(Panel *panel);

    void end_frame();
    void end();

    bool add_toggle(Panel *panel, char *label, bool *state);
    bool add_toggle(Panel *panel, char *label, int *state);
    bool add_slider(Panel *panel, char *label, float *pos, float min, float max);
    bool add_slider(Panel *panel, char *label, int *pos, int min, int max);
    bool add_combobox(Panel *panel, char *label, char **values, int value_count, int *selected_value, bool *expanded);
    bool add_function_plot(Panel *panel, char *label, float *x, float *y, int point_count, float *select_x, float select_y);
    bool add_textbox(Panel *panel, char *label, char *text, int buffer_size, int *cursor_position);

    void release();

    // UI looks control functions.
    void set_background_opacity(float opacity);
    void invert_colors();

    // These functions enable turning UI on/off as a listener for inputs
    void set_input_responsive(bool is_responsive);
    bool is_input_responsive();

    bool is_registering_input();

    // Getters for values on which UI operates
    float get_screen_width();
    Font *get_font();
}

#ifdef CPPLIB_UI_IMPL
#include "ui.cpp"
#endif