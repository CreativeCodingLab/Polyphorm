#include "input.h"

// Input state variables
bool mouse_lbutton_pressed   = false;
bool mouse_lbutton_down      = false;
bool mouse_rbutton_pressed   = false;
bool mouse_rbutton_down      = false;
Vector2 mouse_position_       = Vector2(-1,-1);
Vector2 mouse_delta_position_ = Vector2(0.0f,0.0f);

float mouse_scroll_delta_ = 0.0f;

static bool key_down_[100];
static bool key_pressed_[100];

bool ui_active_ = false;

////////////////////////////
/// PUBLIC API
////////////////////////////

void input::reset()
{
    mouse_lbutton_pressed = false;
    mouse_delta_position_ = Vector2(0.0f, 0.0f);
    mouse_scroll_delta_ = 0.0f;
    memset(key_pressed_, 0, sizeof(bool) * 100);
}   

bool input::mouse_left_button_pressed()
{
    return mouse_lbutton_pressed;
}

bool input::mouse_left_button_down()
{
    return mouse_lbutton_down;
}

bool input::mouse_right_button_pressed()
{
    return mouse_rbutton_pressed;
}

bool input::mouse_right_button_down()
{
    return mouse_rbutton_down;
}

Vector2 input::mouse_position()
{
    return mouse_position_;
}

Vector2 input::mouse_delta_position()
{
    return mouse_delta_position_;
}

void input::set_mouse_left_button_down()
{
    if(!mouse_lbutton_down) mouse_lbutton_pressed = true;
    mouse_lbutton_down = true;
}

void input::set_mouse_left_button_up()
{
    mouse_lbutton_down = false;
}

void input::set_mouse_right_button_down()
{
    if(!mouse_rbutton_down) mouse_rbutton_pressed = true;
    mouse_rbutton_down = true;
}

void input::set_mouse_right_button_up()
{
    mouse_rbutton_down = false;
}

void input::set_mouse_position(Vector2 position)
{
    // Don't update delta on the first frame (initial position (-1, -1))
    if(mouse_position_.x > 0.0f && mouse_position_.y > 0.0f)
    {
        mouse_delta_position_ = position - mouse_position_;
    }
    mouse_position_ = position;
}

void input::set_mouse_scroll_delta(float delta)
{
    mouse_scroll_delta_ = delta;
}

float input::mouse_scroll_delta()
{
    return mouse_scroll_delta_;
}

void input::set_key_down(KeyCode code)
{
    if(!key_down_[code]) key_pressed_[code] = true;
    key_down_[code] = true;
}

void input::set_key_up(KeyCode code)
{
    key_down_[code] = false;
}

bool input::key_pressed(KeyCode code)
{
    return key_pressed_[code];
}

void input::register_event(Event *event)
{
    switch(event->type)
    {
        case MOUSE_MOVE:
        {
            MouseMoveData *data = (MouseMoveData *)event->data;
            input::set_mouse_position(Vector2(data->x, data->y));
        }
        break;
        case MOUSE_LBUTTON_DOWN:
        {
            input::set_mouse_left_button_down();
        }
        break;
        case MOUSE_LBUTTON_UP:
        {
            input::set_mouse_left_button_up();
        }
        break;
        case MOUSE_RBUTTON_DOWN:
        {
            input::set_mouse_right_button_down();
        }
        break;
        case MOUSE_RBUTTON_UP:
        {
            input::set_mouse_right_button_up();
        }
        break;
        case MOUSE_WHEEL:
        {
		    MouseWheelData *data = (MouseWheelData *)event->data;
            input::set_mouse_scroll_delta(data->delta);
        }
        break;
        case KEY_DOWN:
        {
            KeyPressedData *key_data = (KeyPressedData *)event->data;
            input::set_key_down(key_data->code);
        }
        break;
        case KEY_UP:
        {
            KeyPressedData *key_data = (KeyPressedData *)event->data;
            input::set_key_up(key_data->code);
        }
        break;
    }
    
}