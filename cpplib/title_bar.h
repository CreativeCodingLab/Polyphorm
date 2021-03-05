#pragma once
#include <Windows.h>

namespace title_bar {
    void init(HWND window, bool resizable=false);
    void draw();
    void invert_colors();
}

#ifdef CPPLIB_TITLEBAR_IMPL
#include "title_bar.cpp"
#endif