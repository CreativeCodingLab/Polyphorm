#pragma once
#include <Windows.h>
#include <stdint.h>

#define IS_WINDOW_VALID(window) (!(window.window_handle == INVALID_HANDLE_VALUE))

// Represents current window
struct Window
{
	HWND window_handle;
	uint32_t window_width;
	uint32_t window_height;
};

/////////////////////////////////////////
// Event system specific structures
/////////////////////////////////////////
/*
Example of usage of the event system:

// Each frame
Event event;
while(platform::get_event(&event)) // Get all the events accumulated since the last frame
{
	// Check for event type
    switch(event.type)
	{
		case MOUSE_MOVE:
			// Since we now know that event is of MOUSE_MOVE type, we can just cast
			// Event's data member into MouseMoveData structure and read it that way.
			MouseMoveData *mouse_data = (MouseMoveData *)event.data;
		break;
	}
}
*/

// All the event types
enum EventType
{
	EMPTY = 0,
	MOUSE_MOVE,
	MOUSE_LBUTTON_DOWN,
	MOUSE_LBUTTON_UP,
	MOUSE_RBUTTON_DOWN,
	MOUSE_RBUTTON_UP,	
	KEY_DOWN,
	KEY_UP,
	MOUSE_WHEEL,
	EXIT
};

// Event specific data structures

struct MouseMoveData
{
	float x;
	float y;
};

enum KeyCode
{
	ESC = 0,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	NUM1,
	NUM2,
	NUM3,
	NUM4,
	NUM5,
	NUM6,
	OTHER,
};

struct KeyPressedData
{
	KeyCode code;
};

struct MouseWheelData
{
	float delta;
};

// Event data struct
struct Event
{
	EventType type;
	char data[32] = {};
};

// Ticks represent CPU ticks
typedef LARGE_INTEGER Ticks;

// `platform` namespace handles interfacing with windows API, with the exception of file system interface
namespace platform
{
	// Create and return windows with specific name and dimensions
	Window get_window(char *window_name, uint32_t window_width, uint32_t window_height);
	bool set_window_title(Window &window, const char *window_title);

	// Check if window is valid
	bool is_window_valid(Window *window);

	// Get next Event, should be called per frame until false is returned
	bool get_event(Event *event);

	// Cursor manipulation interface
	void show_cursor();
	void hide_cursor();
	
	// Get number of ticks since startup
	Ticks get_ticks();

	// Get tick frequency
	Ticks get_tick_frequency();

	// Compute time difference between two Ticks (number of ticks) and a tick update frequency
	float get_dt_from_tick_difference(Ticks t1, Ticks t2, Ticks frequency);
}

/////////////////////////////////////////
/// Timer API
/////////////////////////////////////////

// Timer is used for simple timing purposes
struct Timer
{
    Ticks frequency;
    Ticks start;
};

namespace timer
{
	// Create a new timer
    Timer get();

	// Start timing
    void start(Timer *timer);

	// Return time since the start
    float end(Timer *timer);
	
	// Return the time since the start and start measuring time from now
    float checkpoint(Timer *timer);
}