#include "font.h"
#include "memory.h"

#ifdef CPPLIB_DEBUG_PRINTS
#include "logging.h"
#define PRINT_DEBUG(message, ...) logging::print_error(message, ##__VA_ARGS__)
#else
#define PRINT_DEBUG(message, ...)
#endif

// Globally unique objects
FT_Library ft_library;

bool font::init()
{
    int32_t error = FT_Init_FreeType(&ft_library);
    if (error) {
        PRINT_DEBUG("Error initializing FreeType library!");
        return false;
    }
    return true;
}

Font font::get(uint8_t *data, int32_t data_size, int32_t size, int32_t texture_size)
{
    Font font = {};
    
    // Store temp allocator stack state    
    memory::push_temp_state();

    // Allocate memory for the texture
    uint8_t *font_buffer = memory::alloc_temp<uint8_t>(texture_size * texture_size);
    FT_Face face;
    int32_t error = FT_New_Memory_Face(ft_library, data, data_size, 0, &face);
    if (error) {
        PRINT_DEBUG("Error creating font face!");
        return Font{};
    }

    int32_t supersampling = 1;
    error = FT_Set_Pixel_Sizes(face, 0, size);
    if (error)
    {
        PRINT_DEBUG("Error setting pixel size!");
        return Font{};
    }

    int32_t row_height = face->size->metrics.height / 64;
    int32_t ascender = face->size->metrics.ascender / 64;
    int32_t descender = face->size->metrics.descender / 64;
    int32_t x = 0, y = 0;

    font.row_height = (float)row_height;
    font.top_pad = (float)(row_height - (ascender - descender));
    font.scale = 1.0f;
    font.face = face;
 
    for (unsigned char c = 32; c < 128; ++c)
    {
        // Load character c
        error = FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT);
        if(error)
        {
            PRINT_DEBUG("Error loading character %c!", c);
            return Font{};
        }

        // Get glyph specific metrics
        int x_offset = face->glyph->bitmap_left;
        int y_offset = ascender - face->glyph->bitmap_top;
        int advance = face->glyph->advance.x >> 6;

        // Get bitmap parameters
        int bitmap_width = face->glyph->bitmap.width, bitmap_height = face->glyph->bitmap.rows;
        int pitch = face->glyph->bitmap.pitch;

        // In case we don't fit in the row, let's move to next row
        if(x > texture_size - bitmap_width)
        {
            x = 0;
            y += row_height;
        }

        // Copy over bitmap
        for(int32_t r = 0; r < bitmap_height; ++r)
        {
            uint8_t *source = face->glyph->bitmap.buffer + pitch * r;
            uint8_t *dest = font_buffer + (y + r) * texture_size + x;
            memcpy(dest, source, bitmap_width);
        }

        // Store glyph settings
        font.glyphs[c - 32] = {x, y, bitmap_width, bitmap_height, x_offset, y_offset, advance};

        // Move in the bitmap
        x += bitmap_width;
    }

    // Initialize D3D texture for the Font
    font.texture = graphics::get_texture2D(font_buffer, texture_size, texture_size, DXGI_FORMAT_R8_UNORM, 1);
    if(!graphics::is_ready(&font.texture))
    {
        PRINT_DEBUG("Could not create texture for font.");
        return Font{};
    }

    // Restore memory state of the temp allocator
    memory::pop_temp_state();

    return font;
}

float font::get_kerning(Font *font, char c1, char c2)
{
    FT_Vector kerning;
    int32_t left_glyph_index = FT_Get_Char_Index(font->face, c1);
    int32_t right_glyph_index = FT_Get_Char_Index(font->face, c2);
    FT_Get_Kerning(font->face, left_glyph_index, right_glyph_index, FT_KERNING_DEFAULT, &kerning);
    return (float)kerning.x;
}

float font::get_string_width(const char *string, Font *font)
{
    float width = 0;
    int i = 0;
    while(string[i])
    {
        // Get a glyph for the current character
        char c = string[i];
        Glyph glyph = font->glyphs[c - 32];

        // Increment width by character's advance. This should be more precise than taking it's bitmap's width.
        width += glyph.advance;
        
        // Take kerning into consideration
        if (string[i+1])
            width += font::get_kerning(font, c, string[i+1]);

        // Next letter
        i++;
    }

    return width;
}

float font::get_row_height(Font *font)
{
    return font->row_height;
}

void font::release(Font *font)
{
    graphics::release(&font->texture);
}
