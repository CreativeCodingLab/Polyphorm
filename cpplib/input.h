#pragma once
#include "platform.h"
#include "maths.h"

// Input namespace handles input state
//
// Usage:
// Call `register_event` on all the events from `platform::get_event` each frame
namespace input {
    // Reset input states that are dependent on state change
    // e.g. - pressed event, mouse delta position....
    void reset();

    // Return if left mouse button has been pressed this frame (now down, previous frame up)
    bool mouse_left_button_pressed();

    // Return if left mouse button is down
    bool mouse_left_button_down();

    // Return if right mouse button is down
    bool mouse_right_button_down();
    
    // Get last mouse position registered
    float mouse_position_x();
    float mouse_position_y();

    // Get difference between last registered position and the one before
    // NOTE: this is zeroed after calling reset()
    float mouse_delta_position_x();
    float mouse_delta_position_y();

    // Get difference between last registered position and the one before
    // NOTE: this is zeroed after calling reset()
    Vector2 mouse_delta_position();

    // Get current movement of mouse scroll wheel
    // NOTE: this is zeroed after calling reset()
    float mouse_scroll_delta();

    // Return if key with specific KeyCode was pressed this frame
    bool key_pressed(KeyCode code);

    // Return if key with specific KeyCode is down
    bool key_down(KeyCode code);

    // Populate buffer with all the characters entered since the reset was called.
    int characters_entered(char *buffer);

    // Process Event, disregards events irrelevant to input
    void register_event(Event *event);
}

#ifdef CPPLIB_INPUT_IMPL
#include "input.cpp"
#endif