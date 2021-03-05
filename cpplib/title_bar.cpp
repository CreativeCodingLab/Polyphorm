#include "title_bar.h"
#include "ui.h"

namespace title_bar {

// Visual constants.
const int CLOSE_BUTTON_RADIUS = 8;
const int CLOSE_BUTTON_PADDING = 20;
const int TITLE_BAR_HEIGHT = CLOSE_BUTTON_PADDING * 2;
const int RESIZE_BORDER_SIZE = 6;

// Resize border hover bitfield flags.
const uint8_t LEFT_RESIZE_HOVER = 1;
const uint8_t RIGHT_RESIZE_HOVER = 2;
const uint8_t TOP_RESIZE_HOVER = 4;
const uint8_t BOTTOM_RESIZE_HOVER = 8;

// Title bar hover bitfield flags.
const uint8_t CLOSE_BUTTON_HOVER = 1;
const uint8_t TITLE_BAR_HOVER = 2;

// We need to store the title bar hover flags for rendering step.
uint8_t title_bar_hover_flags = 0;

// We store the window handle so we can query for window width in our drawing procedure.
HWND window = NULL;

// We store the previous WndProc, just so we can default to it for non-overriden message handling.
WNDPROC previous_wnd_proc;

// We store whether the window should be resizable.
bool resizable;

// Colors.
Vector3 BASE_COLOR = Vector3(1, 1, 1);
Vector3 CLOSE_BUTTON_TITLE_BAR_INACTIVE_COLOR = Vector3(0.5f, 0.5f, 0.5f);
Vector3 CLOSE_BUTTON_COLOR_HOVER = Vector3(1.0f, 32.0f / 255.0f, 32.0f / 255.0f);


LRESULT CALLBACK title_bar_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
	case WM_NCHITTEST:  
	{  
        // Set up hover flags.
        title_bar_hover_flags = 0;
        
        // Get mouse position within the window.
        POINT mouse_position = { LOWORD(lparam), HIWORD(lparam) };  
		ScreenToClient(hwnd, &mouse_position);  

        // Get window dimensions.
		RECT window_rect;
		GetClientRect(hwnd, &window_rect);
        int32_t window_width = window_rect.right - window_rect.left;
        int32_t window_height = window_rect.bottom - window_rect.top;

        // Check if mouse is over the close button.
        POINT close_button_position = { LONG(window_width - CLOSE_BUTTON_PADDING), LONG(CLOSE_BUTTON_PADDING) };
        int32_t dx = mouse_position.x - close_button_position.x;
        int32_t dy = mouse_position.y - close_button_position.y;
        if(dx * dx + dy * dy < CLOSE_BUTTON_RADIUS * CLOSE_BUTTON_RADIUS) {
            title_bar_hover_flags |= CLOSE_BUTTON_HOVER | TITLE_BAR_HOVER;
            return HTCLOSE;
        }

        // Check if mouse is over the title bar.
        RECT title_bar_rect = {
            0, 0,
            LONG(window_width), LONG(TITLE_BAR_HEIGHT),
        };
        if(PtInRect(&title_bar_rect, mouse_position)) {
            title_bar_hover_flags |= TITLE_BAR_HOVER;
            return HTCAPTION;
        }

        if(title_bar::resizable) {
            // Check if mouse is over the resizing border. 
            uint8_t border_flags = 0;
            border_flags |= (mouse_position.x < RESIZE_BORDER_SIZE) * LEFT_RESIZE_HOVER;
            border_flags |= (mouse_position.x > window_width - RESIZE_BORDER_SIZE) * RIGHT_RESIZE_HOVER;
            border_flags |= (mouse_position.y < RESIZE_BORDER_SIZE) * TOP_RESIZE_HOVER;
            border_flags |= (mouse_position.y > window_height - RESIZE_BORDER_SIZE) * BOTTOM_RESIZE_HOVER;
            switch(border_flags) {
                case (LEFT_RESIZE_HOVER | BOTTOM_RESIZE_HOVER): return HTBOTTOMLEFT;
                case (LEFT_RESIZE_HOVER | TOP_RESIZE_HOVER): return HTTOPLEFT;
                case (RIGHT_RESIZE_HOVER | BOTTOM_RESIZE_HOVER): return HTBOTTOMRIGHT;
                case (RIGHT_RESIZE_HOVER | TOP_RESIZE_HOVER): return HTTOPRIGHT;
                case (LEFT_RESIZE_HOVER): return HTLEFT;
                case (RIGHT_RESIZE_HOVER): return HTRIGHT;
                case (TOP_RESIZE_HOVER): return HTTOP;
                case (BOTTOM_RESIZE_HOVER): return HTBOTTOM;
            }
        }

        // In case no areas have been hit, we use default handling procedure.
		return DefWindowProc(hwnd, message, wparam, lparam);  
	} break;
    case WM_NCLBUTTONDOWN:
    {
        // This is just to communicate that we're processing the inputs and so
        // the WM_NCLBUTTONUP message should be sent.
        switch(wparam) {
            case HTCLOSE: return 0;
            default: return CallWindowProcA(previous_wnd_proc, hwnd, message, wparam, lparam);
        }
    }
    break;
    case WM_NCLBUTTONUP:
    {
        // Execute button behavior.
        switch(wparam) {
            case HTCLOSE: PostQuitMessage(0); return 0;
            default: return CallWindowProcA(previous_wnd_proc, hwnd, message, wparam, lparam);
        }
    }
    break;
	default:
        return CallWindowProcA(previous_wnd_proc, hwnd, message, wparam, lparam);
	}
	return 0;
}

}

void title_bar::draw() {
    // Get window width.
    RECT window_rect;
    GetClientRect(title_bar::window, &window_rect);
    int32_t window_width = window_rect.right - window_rect.left;

    // Set up title bar parameters.
    Vector4 title_bar_color = Vector4(BASE_COLOR, title_bar_hover_flags & TITLE_BAR_HOVER ? 0.3f : 0.15f);
    float shading_period = 10.0f;
    float shading_width = 5.0f;

    // Draw the title bar rectangle.
    ui::draw_rect(Vector2(0, 0), float(window_width), TITLE_BAR_HEIGHT, title_bar_color, LINES);

    // Draw the close button.
    {
        // Set up close button's position and color.
        Vector2 close_button_position = Vector2(float(window_width) - float(CLOSE_BUTTON_PADDING), float(CLOSE_BUTTON_PADDING));
        Vector4 close_button_color = Vector4(CLOSE_BUTTON_TITLE_BAR_INACTIVE_COLOR, 1.0f);
        if(title_bar_hover_flags & TITLE_BAR_HOVER) {
            close_button_color = Vector4(BASE_COLOR, 1.0f);
        }
        if(title_bar_hover_flags & CLOSE_BUTTON_HOVER) {
            close_button_color = Vector4(CLOSE_BUTTON_COLOR_HOVER, 1.0f);
        }

        // Cross sizes.
        // `CLOSE_BUTTON_RADIUS` specifies length diagonally, so to get to one end point, we have to move
        // by `radius / sqrt(2.0)` in both x and y axes - "cross side".
        float cross_side = CLOSE_BUTTON_RADIUS / math::sqrt(2.0f);
        float cross_width = cross_side;

        // Construct cross end points.
        Vector2 points[] = {
            close_button_position + Vector2(-cross_side, cross_side),
            close_button_position + Vector2(cross_side, -cross_side),
            close_button_position + Vector2(-cross_side, -cross_side),
            close_button_position + Vector2(cross_side, cross_side)
        };

        // Draw the cross.
        ui::draw_line(points, 2, cross_width, close_button_color);
        ui::draw_line(points + 2, 2, cross_width, close_button_color);
    }
}

void title_bar::init(HWND window, bool resizable) {
    // Save properties.
    title_bar::window = window;
    title_bar::resizable = resizable;

    // Set new WndProc for the window (and remember the old one).
    previous_wnd_proc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)&title_bar_window_proc);
}

void title_bar::invert_colors() {
    BASE_COLOR.x = 1.0f - BASE_COLOR.x;
    BASE_COLOR.y = 1.0f - BASE_COLOR.y;
    BASE_COLOR.z = 1.0f - BASE_COLOR.z;
}