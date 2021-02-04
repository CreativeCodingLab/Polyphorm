#pragma once
#include <stdint.h>
#include "ft2build.h"
#include "graphics.h"
#include FT_FREETYPE_H

// Glyph specifies information aobut a single letter at a single scale
struct Glyph {
    int bitmap_x, bitmap_y;
    int bitmap_width, bitmap_height;
    int x_offset, y_offset;
    int advance;
};

// Font contains data for rendering text at specific scale
struct Font {
    Glyph glyphs[96];
    float row_height;
    float top_pad;
    float scale;
    uint8_t *bitmap;
    int bitmap_width, bitmap_height;
    Texture2D texture;
    FT_Face face;
};

// font namespace handles loading Fonts and extracting information from it
namespace font {
    /*
    Return initialized Font object

    Args:
        - data: binary data from .ttf/.otf file
        - data_size: size of data block in bytes
        - size: height of the font in pixels
        - bitmap_size: size of a side of bitmap that stores font, in pixels
    */
    Font get(uint8_t *data, int32_t data_size, int32_t size, int32_t bitmap_size);

    // Initialize font rasterization code (FreeType2)
    bool init();

    // Return height of a single row for a Font
    float get_row_height(Font *font);

    // Return width of a string
    float get_string_width(char *string, Font *font);
    float get_string_width(const char *string, Font *font);
    float get_string_width(char *string, int string_length, Font *font);

    // Get kerning between two letters of a Font
    float get_kerning(Font *font, char c1, char c2);

    // Release a Font object
    void release(Font *font);
}

#ifdef CPPLIB_FONT_IMPL
#include "font.cpp"
#endif