#include "ui.h"
#include "font.h"
#include "graphics.h"
#include "array.h"
#include "input.h"
#include "file_system.h"
#include <cassert>

static ConstantBuffer buffer_rect;
static ConstantBuffer buffer_pv;
static ConstantBuffer buffer_model;
static ConstantBuffer buffer_color;

static VertexShader vertex_shader_font;
static PixelShader pixel_shader_font;
static VertexShader vertex_shader_rect;
static PixelShader pixel_shader_rect;

static TextureSampler texture_sampler;

static Mesh quad_mesh;

static Font font_ui;

static File font_file;

static bool is_input_rensposive_ = true;
static bool is_registering_input_ = false;

char vertex_shader_font_string[] = 
"struct VertexInput"
"{"
"	float4 position: POSITION;"
"	float2 texcoord: TEXCOORD;"
"};"
""
"struct VertexOutput"
"{"
"	float4 svPosition: SV_POSITION;"
"	float2 texcoord: TEXCOORD;"
"};"
""
"cbuffer PVBuffer : register(b0)"
"{"
"	matrix projection;"
"	matrix view;"
"};"
""
"cbuffer PVBuffer : register(b1)"
"{"
"	matrix model;"
"};"
""
"cbuffer SourceRectBuffer: register(b6)"
"{"
"    float4 source_rect;"
"}"
""
"VertexOutput main(VertexInput input)"
"{"
"	VertexOutput result;"
""
"	result.svPosition = mul(projection, mul(view, mul(model, input.position)));"
"	result.texcoord = input.texcoord * source_rect.zw + source_rect.xy;"
""
"	return result;"
"}";

char pixel_shader_font_string[] = 
"struct PixelInput"
"{"
"	float4 svPosition: SV_POSITION;"
"	float2 texcoord: TEXCOORD;"
"};"
""
"SamplerState texSampler: register(s0);"
"Texture2D tex: register(t0);"
""
"cbuffer ColorBuffer: register(b7)"
"{"
"	float4 color;"
"}"
""
"float4 main(PixelInput input) : SV_TARGET"
"{"
"	float alpha = tex.Sample(texSampler, input.texcoord).r;"
"	float4 output = float4(color.xyz, color.w * alpha);"
"	return output;"
"}";

char vertex_shader_rect_string[] =
"struct VertexInput"
"{"
"	float4 position: POSITION;"
"};"
""
"struct VertexOutput"
"{"
"	float4 svPosition: SV_POSITION;"
"};"
""
"cbuffer PVBuffer : register(b0)"
"{"
"	matrix projection;"
"	matrix view;"
"};"
""
"cbuffer PVBuffer : register(b1)"
"{"
"	matrix model;"
"};"
""
"VertexOutput main(VertexInput input)"
"{"
"	VertexOutput result;"
""
"	result.svPosition = mul(projection, mul(view, mul(model, input.position)));"
""
"	return result;"
"}";

char pixel_shader_rect_string[] = 
"struct PixelInput"
"{"
"	float4 svPosition: SV_POSITION;"
"};"
""
"cbuffer ColorBuffer: register(b7)"
"{"
"	float4 color;"
"}"
""
"float4 main(PixelInput input) : SV_TARGET"
"{"
"	return color;"
"}";

struct TextItem
{
    Vector4 color;
    Vector2 pos;
    char text[20];
    Vector2 origin = Vector2(0,0);
};

struct RectItem
{
    Vector4 color;
    Vector2 pos;
    Vector2 size;
};

static Array<TextItem> text_items;
static Array<RectItem> rect_items;
static Array<RectItem> rect_items_bg;

int32_t hash_string(char *string)
{
    int32_t hash_value = 5381;
    while(*string)
    {
        hash_value = ((hash_value << 5) + hash_value) + *string;
        string++;
    }

    return hash_value + 1;
}

static float quad_vertices[] = {
    -1.0, 1.0, 0.0, 1.0, // LEFT TOP 1 
    0.0, 0.0,
    1.0, 1.0, 0.0, 1.0, // RIGHT TOP 2
    1.0, 0.0,
    -1.0, -1.0, 0.0, 1.0, // LEFT BOT 3
    0.0, 1.0,
    1.0, -1.0, 0.0, 1.0, // RIGHT BOT 4
    1.0, 1.0
};

static uint16_t quad_indices[] = {
    2, 3, 1,
    2, 1, 0
};

static float screen_width = -1, screen_height = -1;

const float FONT_TEXTURE_SIZE = 256.0f;
const int32_t FONT_HEIGHT = 12;

#define ASSERT_SCREEN_SIZE
// #define ASSERT_SCREEN_SIZE assert(screen_width > 0 && screen_height > 0)

void ui::init(float screen_width_ui, float screen_height_ui)
{
    // Set screen size
    screen_width = screen_width_ui;
    screen_height = screen_height_ui;

    // Create constant buffers
    buffer_model = graphics::get_constant_buffer(sizeof(Matrix4x4));
    buffer_pv = graphics::get_constant_buffer(sizeof(Matrix4x4) * 2);
    buffer_rect = graphics::get_constant_buffer(sizeof(Vector4));
    buffer_color = graphics::get_constant_buffer(sizeof(Vector4));

    // Assert constant buffers are ready
    assert(graphics::is_ready(&buffer_model));
    assert(graphics::is_ready(&buffer_pv));
    assert(graphics::is_ready(&buffer_rect));
    assert(graphics::is_ready(&buffer_color));

    // Create mesh
    quad_mesh = graphics::get_mesh(quad_vertices, 4, sizeof(float) * 6, quad_indices, 6, 2);
    assert(graphics::is_ready(&quad_mesh));
    
    // Create shaders
    vertex_shader_font = graphics::get_vertex_shader_from_code(vertex_shader_font_string, ARRAYSIZE(vertex_shader_font_string));
    pixel_shader_font = graphics::get_pixel_shader_from_code(pixel_shader_font_string, ARRAYSIZE(pixel_shader_font_string));
    vertex_shader_rect = graphics::get_vertex_shader_from_code(vertex_shader_rect_string, ARRAYSIZE(vertex_shader_rect_string));
    pixel_shader_rect = graphics::get_pixel_shader_from_code(pixel_shader_rect_string, ARRAYSIZE(pixel_shader_rect_string));
    assert(graphics::is_ready(&vertex_shader_font));
    assert(graphics::is_ready(&pixel_shader_font));
    assert(graphics::is_ready(&vertex_shader_rect));
    assert(graphics::is_ready(&pixel_shader_rect));

    // Create texture sampler
    texture_sampler = graphics::get_texture_sampler(SampleMode::CLAMP);
    assert(graphics::is_ready(&texture_sampler));

    // Init font
    font_file = file_system::read_file("renner-book.otf");
    
    assert(font_file.data);
    font_ui = font::get((uint8_t *)font_file.data, font_file.size, FONT_HEIGHT, (uint32_t)FONT_TEXTURE_SIZE);
    assert(graphics::is_ready(&font_ui.texture));

    // Init rendering arrays
    array::init(&text_items, 100);
    array::init(&rect_items, 100);
    array::init(&rect_items_bg, 100);
}

void ui::draw_text(const char *text, Font *font, float x, float y, Vector4 color, Vector2 origin)
{
    ASSERT_SCREEN_SIZE;

    // Set font shaders
    graphics::set_pixel_shader(&pixel_shader_font);
    graphics::set_vertex_shader(&vertex_shader_font);
    
    // Set font texture
    graphics::set_texture(&font->texture, 0);
    graphics::set_texture_sampler(&texture_sampler, 0);
    
    // Set alpha blending state
    BlendType old_blend_state = graphics::get_blend_state();
    graphics::set_blend_state(BlendType::ALPHA);

    // Set constant buffers
    graphics::set_constant_buffer(&buffer_pv, 0);
    graphics::set_constant_buffer(&buffer_model, 1);
    graphics::set_constant_buffer(&buffer_rect, 6);
    graphics::set_constant_buffer(&buffer_color, 7);

    // Update constant buffer values
    Matrix4x4 pv_matrices[2] = {
        math::get_orthographics_projection_dx_rh(0, screen_width, 0, screen_height, -1, 1),
        math::get_identity()
    };
    graphics::update_constant_buffer(&buffer_pv, pv_matrices);
    graphics::update_constant_buffer(&buffer_color, &color);

    // Get final text dimensions
    float text_width = font::get_string_width(text, font);
    float text_height = font::get_row_height(font);

    // Adjust starting point based on the origin
    x = math::floor(x - origin.x * text_width);
    y = math::floor(y - origin.y * text_height);

    y += font->top_pad;
    int i = 0;
    while(text[i])
    {
        char c = text[i];
        Glyph glyph = font->glyphs[c - 32];
        
        // Set up source rectangle
        float rel_x = glyph.bitmap_x / FONT_TEXTURE_SIZE;
        float rel_y = glyph.bitmap_y / FONT_TEXTURE_SIZE;
        float rel_width = glyph.bitmap_width / FONT_TEXTURE_SIZE;
        float rel_height = glyph.bitmap_height / FONT_TEXTURE_SIZE;
        Vector4 source_rect = {rel_x, rel_y, rel_width, rel_height};
        graphics::update_constant_buffer(&buffer_rect, &source_rect);

        // Get final letter start
        float final_x = x + glyph.x_offset;
        float final_y = y + glyph.y_offset;

        // Set up model matrix
        // TODO: solve y axis downwards in an elegant way
        Matrix4x4 model_matrix = math::get_translation(final_x, screen_height - final_y, 0) * math::get_scale((float)glyph.bitmap_width, (float)glyph.bitmap_height, 1.0f) * math::get_translation(Vector3(0.5f, -0.5f, 0.0f)) * math::get_scale(0.5f);
        graphics::update_constant_buffer(&buffer_model, &model_matrix);

        graphics::draw_mesh(&quad_mesh);

        // Update current position for next letter
        if (text[i+1])
            x += font::get_kerning(font, c, text[i+1]);
        x += glyph.advance;
        i++;
    }

    // Reset previous blend state
    graphics::set_blend_state(old_blend_state);
};

void ui::draw_text(const char *text, Font *font, Vector2 pos, Vector4 color, Vector2 origin)
{
    ui::draw_text(text, font, pos.x, pos.y, color, origin);
}

void ui::draw_text(const char *text, Vector2 pos, Vector4 color, Vector2 origin)
{
    ui::draw_text(text, &font_ui, pos.x, pos.y, color, origin);
}

void ui::draw_rect(float x, float y, float width, float height, Vector4 color)
{
    ASSERT_SCREEN_SIZE;

    // Set rect shaders
    graphics::set_pixel_shader(&pixel_shader_rect);
    graphics::set_vertex_shader(&vertex_shader_rect);

    // Set alpha blending state
    BlendType old_blend_state = graphics::get_blend_state();
    graphics::set_blend_state(BlendType::ALPHA);

    // Set constant bufers
    graphics::set_constant_buffer(&buffer_pv, 0);
    graphics::set_constant_buffer(&buffer_model, 1);
    graphics::set_constant_buffer(&buffer_color, 7);

    // Get constant buffer values
    Matrix4x4 pv_matrices[2] = {
        math::get_orthographics_projection_dx_rh(0, screen_width, 0, screen_height, -1, 1),
        math::get_identity()
    };
    Matrix4x4 model_matrix = math::get_translation(x, screen_height - y, 0) * math::get_scale(width, height, 1.0f) * math::get_translation(Vector3(0.5f, -0.5f, 0.0f)) * math::get_scale(0.5f);

    // Update constant buffers
    graphics::update_constant_buffer(&buffer_pv, pv_matrices);
    graphics::update_constant_buffer(&buffer_color, &color);
    graphics::update_constant_buffer(&buffer_model, &model_matrix);

    // Render rect
    graphics::draw_mesh(&quad_mesh);

    // Reset previous blending state
    graphics::set_blend_state(old_blend_state);
}

void ui::draw_rect(Vector2 pos, float width, float height, Vector4 color)
{
    ui::draw_rect(pos.x, pos.y, width, height, color);
}

const float vertical_padding = 5.0f;
const float horizontal_padding = 15.0f;
const float inner_padding = 5.0f;
static int32_t active_id = -1;
static int32_t hot_id = -1;

#define COLOR(r,g,b,a) Vector4((r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a) / 255.0f)
// This means that the color has been NOT gamma corrected - it was seen displayed incorrectly.
#define COLOR_LINEAR(r,g,b,a) Vector4(math::pow((r) / 255.0f, 2.2f), math::pow((g) / 255.0f, 2.2f), math::pow((b) / 255.0f, 2.2f), math::pow((a) / 255.0f, 2.2f))
static Vector4 color_background = Vector4(0.1f, 0.1f, 0.1f, .8f);
static Vector4 color_foreground = Vector4(0.8f, 0.8f, 0.8f, 0.6f);
static Vector4 color_title = COLOR_LINEAR(28, 224, 180, 255);
static Vector4 color_label = Vector4(1.0f, 1.0f, 1.0f, 0.6f);

Panel ui::start_panel(char *name, Vector2 pos, float width)
{
    Panel panel = {};

    panel.pos = pos;
    panel.width = width;
    panel.item_pos.x = horizontal_padding;
    //panel.item_pos.y = font::get_row_height(&font_ui) + vertical_padding * 2.0f + inner_padding;
    panel.item_pos.y = horizontal_padding;
    panel.name = name;
    
    return panel;
}

Panel ui::start_panel(char *name, float x, float y, float width)
{
    return ui::start_panel(name, Vector2(x,y), width);
}

Vector4 ui::get_panel_rect(Panel *panel) {
    Vector4 result = Vector4(panel->pos.x, panel->pos.y, panel->width, panel->item_pos.y + horizontal_padding - inner_padding);
    return result;
}

void ui::end_panel(Panel *panel)
{
    Vector4 panel_rect = get_panel_rect(panel);
    RectItem panel_bg = {color_background, Vector2(panel_rect.x, panel_rect.y), Vector2(panel_rect.z, panel_rect.w)};
    array::add(&rect_items_bg, panel_bg);

    //float title_bar_height = font::get_row_height(&font_ui) + vertical_padding * 2;
    //RectItem title_bar = {color_foreground, panel->pos, Vector2(panel->width, title_bar_height)};
    //array::add(&rect_items_bg, title_bar);

    Vector2 title_pos = panel->pos + Vector2(horizontal_padding, inner_padding);
    TextItem title = {};
    title.color = color_background;
    title.pos = title_pos;
    memcpy(title.text, panel->name, strlen(panel->name) + 1);
    array::add(&text_items, title);
}

bool is_in_rect(Vector2 position, Vector2 rect_position, Vector2 rect_size)
{
    if (position.x >= rect_position.x && position.x <= rect_position.x + rect_size.x &&
        position.y >= rect_position.y && position.y <= rect_position.y + rect_size.y)
        return true;
    return false;
}

void unset_hot(int32_t item_id)
{
    if(hot_id == item_id)
    {
        hot_id = -1;
    }
}

void unset_active(int32_t item_id)
{
    if(active_id == item_id)
    {
        active_id = -1;
        is_registering_input_ = false;
    }
}

bool is_hot(int32_t item_id)
{
    return item_id == hot_id;
}

bool is_active(int32_t item_id)
{
    return item_id == active_id;
}

void set_hot(int32_t item_id)
{
    if(hot_id == -1)
    {
        hot_id = item_id;
    }
}

void set_active(int32_t item_id)
{
    if(active_id == -1)
    {
        active_id = item_id;
        is_registering_input_ = true;
    }
}

bool ui::add_toggle(Panel *panel, char *label, bool *active)
{
    bool changed = false;
    const float box_middle_to_total = 0.6f;
    
    float height = font::get_row_height(&font_ui);
    Vector2 item_pos = panel->pos + panel->item_pos;
    int32_t toggle_id = hash_string(label);

    Vector2 box_bg_size = Vector2(height, height);
    Vector2 box_bg_pos = item_pos;

    // Check for mouse input
    if(ui::is_input_responsive())
    {
        // Check if mouse over
        Vector2 mouse_position = input::mouse_position();
        if(is_in_rect(mouse_position, box_bg_pos, box_bg_size))
        {
            set_hot(toggle_id);
        }
        // Remove hotness only if this toggle was hot before
        else
        {
            unset_hot(toggle_id);
        }

        // If toggle is hot, check for mouse press
        if (is_hot(toggle_id) && input::mouse_left_button_pressed())
        {
            *active = !(*active);
            changed = true;

            set_active(toggle_id);
        }
        else if(is_active(toggle_id) && !input::mouse_left_button_down())
        {
            unset_active(toggle_id);
        }
    }
    else
    {
        unset_hot(toggle_id);
    }

    // Toggle box background
    Vector4 color_box = color_foreground;
    Vector4 color_middle = color_background;
    if (is_hot(toggle_id))
    {
        color_box *= 0.8f;
        color_middle *= 0.8f;
        color_middle.a = 1.0f;
    }

    // Draw bg rectangle
    RectItem toggle_bg = {color_box, box_bg_pos, box_bg_size};
    array::add(&rect_items, toggle_bg);

    // Active part of toggle box
    if (!*active)
    {
        Vector2 box_fg_size = Vector2(height * box_middle_to_total, height * box_middle_to_total);
        Vector2 box_fg_pos = box_bg_pos + (box_bg_size - box_fg_size) / 2.0f;
        RectItem toggle_fg = {color_middle, box_fg_pos, box_fg_size};
        array::add(&rect_items, toggle_fg);
    }

    // Draw toggle label
    Vector2 text_pos = box_bg_pos + Vector2(inner_padding + box_bg_size.x, 0);
    TextItem toggle_label = {};
    toggle_label.color = color_label;
    toggle_label.pos = text_pos;
    memcpy(toggle_label.text, label, strlen(label) + 1);
    array::add(&text_items, toggle_label);

    // Move current panel item position
    panel->item_pos.y += height + inner_padding;
    return changed;
}

bool ui::add_slider(Panel *panel, char *label, float *pos, float min, float max)
{
    bool changed = false;
    int32_t slider_id = hash_string(label);
    Vector2 item_pos = panel->pos + panel->item_pos;
    float height = font::get_row_height(&font_ui);
    float slider_width = 225.0f;

    // Slider bar
    float slider_start = 0.0f;

    Vector4 slider_bar_color = color_background * 2.0f;
    Vector2 slider_bar_pos = item_pos + Vector2(slider_start, 0.0f);
    Vector2 slider_bar_size = Vector2(slider_width, height);
    RectItem slider_bar = { slider_bar_color, slider_bar_pos, slider_bar_size };
    array::add(&rect_items, slider_bar);

    Vector4 slider_color = color_foreground;
    Vector2 slider_size = Vector2(height, height);
    float slider_x = (*pos - min) / (max - min) * (slider_width - slider_size.x) + slider_bar_pos.x + slider_size.x * 0.5f;
    Vector2 slider_pos = Vector2(slider_x - slider_size.x * 0.5f, item_pos.y);

    // Max number
    Vector2 current_pos = Vector2(slider_bar_pos.x + slider_bar_size.x / 2.0f, item_pos.y);
    TextItem current_label = {};
    current_label.color = color_label;
    current_label.pos = current_pos; 
    current_label.origin = Vector2(0.5f, 0.0f);
    sprintf_s(current_label.text, ARRAYSIZE(current_label.text), "%.2f", *pos);
    array::add(&text_items, current_label);

    // Check for mouse input
    if(ui::is_input_responsive())
    {
        Vector2 mouse_position = input::mouse_position();
        if(is_in_rect(mouse_position, slider_pos, slider_size))
        {       
            set_hot(slider_id);
        }
        else if(!is_active(slider_id))
        {
            unset_hot(slider_id);
        }

        Vector2 overall_slider_size = Vector2(slider_bar_size.x, slider_size.y);
        Vector2 overall_slider_pos = Vector2(slider_bar_pos.x, slider_pos.y);

        if((is_hot(slider_id) || is_in_rect(mouse_position, overall_slider_pos, overall_slider_size)) && !is_active(slider_id) && input::mouse_left_button_down())
        {
            set_active(slider_id);
        }
        else if (is_active(slider_id) && !input::mouse_left_button_down())
        {
            unset_active(slider_id);
        }
    }
    else 
    {
        unset_hot(slider_id);
        unset_active(slider_id);
    }

    if (is_hot(slider_id))
    {
        slider_color *= 0.8f;
        slider_color.w = 1.0f;
    }

    if(is_active(slider_id))
    {
        float mouse_x = input::mouse_position().x;
        float mouse_x_rel = (mouse_x - slider_bar_pos.x - slider_size.x * 0.5f) / (slider_bar_size.x - slider_size.x);
        mouse_x_rel = math::clamp(mouse_x_rel, 0.0f, 1.0f);
        
        *pos = mouse_x_rel * (max - min) + min;

        changed = true;
    }

    // Slider
    RectItem slider = { slider_color, slider_pos, slider_size };
    array::add(&rect_items, slider);

    // Slider label
    Vector2 text_pos = Vector2(slider_bar_size.x + slider_bar_pos.x + inner_padding, item_pos.y);
    TextItem slider_label = {};
    slider_label.color = color_label;
    slider_label.pos = text_pos;
    memcpy(slider_label.text, label, strlen(label) + 1);
    array::add(&text_items, slider_label);

    panel->item_pos.y += height + inner_padding;
    return changed;
}

void ui::end()
{
    for(uint32_t i = 0; i < rect_items_bg.count; ++i)
    {
        RectItem *item = &rect_items_bg.data[i];
        ui::draw_rect(item->pos, item->size.x, item->size.y, item->color);
    }

    for(uint32_t i = 0; i < rect_items.count; ++i)
    {
        RectItem *item = &rect_items.data[i];
        ui::draw_rect(item->pos, item->size.x, item->size.y, item->color);
    }

    for(uint32_t i = 0; i < text_items.count; ++i)
    {
        TextItem *item = &text_items.data[i];
        ui::draw_text(item->text, &font_ui, item->pos, item->color, item->origin);
    }

    array::reset(&rect_items_bg);
    array::reset(&rect_items);
    array::reset(&text_items);
}

void ui::set_input_responsive(bool is_responsive)
{
    is_input_rensposive_ = is_responsive;
}

bool ui::is_input_responsive()
{
    return is_input_rensposive_;
}

bool ui::is_registering_input()
{
    return is_registering_input_;
}

float ui::get_screen_width()
{
    return screen_width;
}

Font *ui::get_font()
{
    return &font_ui;
}


void ui::release()
{
    graphics::release(&buffer_rect);
    graphics::release(&buffer_pv);
    graphics::release(&buffer_model);
    graphics::release(&buffer_color);

    graphics::release(&quad_mesh);
    graphics::release(&texture_sampler);

    graphics::release(&vertex_shader_font);
    graphics::release(&pixel_shader_font);
    graphics::release(&vertex_shader_rect);
    graphics::release(&pixel_shader_rect);

    font::release(&font_ui);
    file_system::release_file(font_file);
}