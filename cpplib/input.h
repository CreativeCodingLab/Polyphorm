#pragma once
#include "maths.h"
#include "platform.h"

// Input namespace handles input state
//
// Usage:
// Call `register_event` on all the events from `platform::get_event` each frame
namespace input
{
    // Reset input states that are dependent on state change
    // e.g. - pressed event, mouse delta position....
    void reset();

    // Return if left mouse button has been pressed this frame (now down, previous frame up)
    bool mouse_left_button_pressed();

    // Return if left mouse button is down
    bool mouse_left_button_down();

    // Return if right mouse button has been pressed this frame (now down, previous frame up)
    bool mouse_right_button_pressed();

    // Return if right mouse button is down
    bool mouse_right_button_down();
    
    // Get last mouse position registered
    Vector2 mouse_position();

    // Get difference between last registered position and the one before
    // NOTE: this is zeroed after calling reset()
    Vector2 mouse_delta_position();

    // Get current movement of mouse scroll wheel
    // NOTE: this is zeroed after calling reset()
    float mouse_scroll_delta();

    // Registered left mouse button as being down
    void set_mouse_left_button_down();

    // Registered left mouse button as being u
    void set_mouse_left_button_up();

    // Registered right mouse button as being down
    void set_mouse_right_button_down();

    // Registered right mouse button as being u
    void set_mouse_right_button_up();
    
    // Registered mouse position
    void set_mouse_position(Vector2 position);

    // Register mouse wheel scroll delta
    void set_mouse_scroll_delta(float delta);

    // Registered key with specific KeyCode as being down
    void set_key_down(KeyCode code);

    // Registered key with specific KeyCode as being up
    void set_key_up(KeyCode code);

    // Return if key with specific KeyCode was pressed this frame
    bool key_pressed(KeyCode code);

    // Process Event, disregards events irrelevant to input
    void register_event(Event *event);
}