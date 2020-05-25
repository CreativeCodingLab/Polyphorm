#include <windowsx.h>
#include "platform.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	}
	break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

bool platform::get_event(Event *event)
{
	MSG message;

	event->type = EMPTY;
	bool is_message = PeekMessageA(&message, NULL, 0, 0, PM_REMOVE);
	if(!is_message) return false;
	switch (message.message)
	{
	case WM_INPUT:
	{
		KeyPressedData *data = (KeyPressedData *)event->data;
		
		char buffer[sizeof(RAWINPUT)] = {};
		UINT size = sizeof(RAWINPUT);
		GetRawInputData(reinterpret_cast<HRAWINPUT>(message.lParam), RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));
		
		// extract keyboard raw input data
		RAWINPUT* raw = reinterpret_cast< RAWINPUT*>(buffer);
		if (raw->header.dwType == RIM_TYPEKEYBOARD)
		{
			const RAWKEYBOARD& raw_kb = raw->data.keyboard;
			if (raw_kb.Flags & RI_KEY_BREAK) 
			{ 
				event->type = KEY_UP;
			}
			else
			{
				event->type = KEY_DOWN;
			}

			if (raw_kb.VKey == VK_ESCAPE) data->code = KeyCode::ESC;
			else if(raw_kb.VKey == VK_F1) data->code = KeyCode::F1;
			else if(raw_kb.VKey == VK_F2) data->code = KeyCode::F2;
			else if(raw_kb.VKey == VK_F3) data->code = KeyCode::F3;
			else if(raw_kb.VKey == VK_F4) data->code = KeyCode::F4;
			else if(raw_kb.VKey == VK_F5) data->code = KeyCode::F5;
			else if(raw_kb.VKey == VK_F6) data->code = KeyCode::F6;
			else if(raw_kb.VKey == VK_F7) data->code = KeyCode::F7;
			else if(raw_kb.VKey == VK_F8) data->code = KeyCode::F8;
			else if(raw_kb.VKey == VK_F9) data->code = KeyCode::F9;
			else if(raw_kb.VKey == VK_F10) data->code = KeyCode::F10;
			else if(raw_kb.VKey == '1') data->code = KeyCode::NUM1;
			else if(raw_kb.VKey == '2') data->code = KeyCode::NUM2;
			else if(raw_kb.VKey == '3') data->code = KeyCode::NUM3;
			else if(raw_kb.VKey == '4') data->code = KeyCode::NUM4;
			else if(raw_kb.VKey == '5') data->code = KeyCode::NUM5;
			else if(raw_kb.VKey == '6') data->code = KeyCode::NUM6;
			else data->code = KeyCode::OTHER;
		}
	} break;
	case WM_QUIT:
	{
		event->type = EXIT;
	}
	break;
	case WM_LBUTTONUP:
	{
		// See below in WM_LBUTTONDOWN for what this does.
		HWND window = GetActiveWindow();
		ReleaseCapture();
		
		event->type = MOUSE_LBUTTON_UP;
	} break;
	case WM_LBUTTONDOWN:
	{
		// This deals with situation when you press the button, go outside the window and release.
		// Without this, you wouldn't register BUTTON_UP message.
		HWND window = GetActiveWindow();
		SetCapture(window);

		event->type = MOUSE_LBUTTON_DOWN;
	} break;
	case WM_RBUTTONUP:
	{
		// See below in WM_LBUTTONDOWN for what this does.
		HWND window = GetActiveWindow();
		ReleaseCapture();
		
		event->type = MOUSE_RBUTTON_UP;
	} break;
	case WM_RBUTTONDOWN:
	{
		// This deals with situation when you press the button, go outside the window and release.
		// Without this, you wouldn't register BUTTON_UP message.
		HWND window = GetActiveWindow();
		SetCapture(window);

		event->type = MOUSE_RBUTTON_DOWN;
	} break;
	case WM_MOUSEMOVE:
	{
		event->type = MOUSE_MOVE;
		MouseMoveData *data = (MouseMoveData *)event->data;
		data->x = (float)GET_X_LPARAM(message.lParam);
		data->y = (float)GET_Y_LPARAM(message.lParam);
	} break;
	case WM_MOUSEWHEEL:
	{
		event->type = MOUSE_WHEEL;
		MouseWheelData *data = (MouseWheelData *)event->data;
		data->delta = GET_WHEEL_DELTA_WPARAM(message.wParam) / 120.0f;
	} break;
	default:
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
	
	return true;
}

Window platform::get_window(char *window_name, uint32_t window_width, uint32_t window_height)
{
	HINSTANCE program_instance = GetModuleHandle(0);
	
	WNDCLASSEXA window_class = {};
	window_class.cbSize = sizeof(WNDCLASSEXA);
	window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = &WindowProc;
	window_class.hInstance = (HINSTANCE)program_instance;
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.lpszClassName = "CustomWindowClass";

	Window window = {};
	if (RegisterClassExA(&window_class))
	{
		//SetProcessDPIAware();
		window.window_width = window_width;
		window.window_height = window_height;
		
		DWORD window_flags = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
		RECT window_rect = {0, 0, (LONG)window_width, (LONG)window_height};
		AdjustWindowRect(&window_rect, window_flags, FALSE);
		window_width = window_rect.right - window_rect.left;
		window_height = window_rect.bottom - window_rect.top;
		
		window.window_handle = CreateWindowA("CustomWindowClass", window_name, window_flags, 
											 100, 100, window_width, window_height, NULL, NULL, program_instance, NULL);

		RAWINPUTDEVICE device;
		device.usUsagePage = 0x01;
		device.usUsage = 0x06;
		device.dwFlags = RIDEV_NOLEGACY;        // do not generate legacy messages such as WM_KEYDOWN
		device.hwndTarget = window.window_handle;
		RegisterRawInputDevices(&device, 1, sizeof(device));
	}
	return window;
}

bool platform::set_window_title(Window &window, const char *window_title)
{
	return SetWindowTextA(window.window_handle, window_title);
}

bool platform::is_window_valid(Window *window)
{
	return window->window_handle != INVALID_HANDLE_VALUE;
}

void platform::show_cursor()
{
	ShowCursor(TRUE);
}

void platform::hide_cursor()
{
	while(ShowCursor(FALSE) >= 0);
}

Ticks platform::get_ticks()
{
	LARGE_INTEGER ticks;
	
	QueryPerformanceCounter(&ticks);

	return (Ticks)ticks;
}

Ticks platform::get_tick_frequency()
{
	LARGE_INTEGER frequency;

	QueryPerformanceFrequency(&frequency);

	return (Ticks)frequency;
}

float platform::get_dt_from_tick_difference(Ticks t1, Ticks t2, Ticks frequency)
{
	float dt = (float)((t2.QuadPart - t1.QuadPart) / (double)frequency.QuadPart);
	return dt;
}

Timer timer::get()
{
	Timer timer = {};
	timer.frequency = platform::get_tick_frequency();
	return timer;
}

void timer::start(Timer *timer)
{
	timer->start = platform::get_ticks();
}

float timer::end(Timer *timer)
{
	Ticks current = platform::get_ticks();
	float dt = platform::get_dt_from_tick_difference(timer->start, current, timer->frequency);
	return dt;
}

float timer::checkpoint(Timer *timer)
{
	Ticks current = platform::get_ticks();
	float dt = platform::get_dt_from_tick_difference(timer->start, current, timer->frequency);
	timer->start = current;
	
	return dt;
}