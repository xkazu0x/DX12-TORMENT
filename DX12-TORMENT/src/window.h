#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

struct window_state {
	HINSTANCE instance;
	HWND hwnd;
	int xpos;
	int ypos;
	int width;
	int height;
	const char* name;
};

namespace window {
	bool startup(window_state* win_state);
	void shutdown(window_state* win_state);
	void pump_messages(window_state* win_state);
	void show_window(window_state* win_state);
}