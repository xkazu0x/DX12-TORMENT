#include "window.h"

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

bool window::startup(window_state* win_state) {
	win_state->instance = GetModuleHandleA(0);

	HICON icon = LoadIcon(win_state->instance, IDI_APPLICATION);
	WNDCLASSA wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = win32_process_message;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = win_state->instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = 0;
	wc.lpszClassName = "DIRECTX12_CLASS";

	if (!RegisterClassA(&wc)) {
		MessageBoxA(0, "FAILED TO REGISTER WINDOW CLASS", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	int window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
	int window_ex_style = WS_EX_APPWINDOW;

	window_style |= WS_MAXIMIZEBOX;
	window_style |= WS_MINIMIZEBOX;
	window_style |= WS_THICKFRAME;

	RECT border_rect = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

	win_state->xpos += border_rect.left;
	win_state->ypos += border_rect.top;

	win_state->width += border_rect.right - border_rect.left;
	win_state->height += border_rect.bottom - border_rect.top;

	HWND hwnd = CreateWindowExA(
		window_ex_style, wc.lpszClassName, win_state->name,
		window_style, win_state->xpos, win_state->ypos, win_state->width, win_state->height,
		0, 0, win_state->instance, 0);
	if (!hwnd) {
		MessageBoxA(NULL, "FAILED TO CREATE WINDOW", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	else {
		win_state->hwnd = hwnd;
	}

	return true;
}

void window::shutdown(window_state* win_state) {
	if (win_state->hwnd) {
		DestroyWindow(win_state->hwnd);
		win_state->hwnd = 0;
	}
}

void window::pump_messages(window_state* win_state) {
	MSG message;
	while (PeekMessage(&message, win_state->hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

void window::show_window(window_state* win_state) {
	ShowWindow(win_state->hwnd, SW_SHOW);
}

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	LRESULT result = 0;
	switch (message) {
		case WM_CLOSE:
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			result = DefWindowProcA(hwnd, message, wparam, lparam);
	}
	return result;
}