#include "window.h"
#include "dx12_renderer.h"

struct engine {
	bool quit;
}engine;

static window_state win_state;

int main() {
	win_state.xpos = 100;
	win_state.ypos = 100;
	win_state.width = 1920 / 2;
	win_state.height = 1080 / 2;
	win_state.name = "DIRECTX12 TORMENT";

	if (!window::startup(&win_state)) {
		MessageBoxA(0, "FAILED TO STARTUP WINDOW", "ERROR", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	dx12::initialize(&win_state);
	window::show_window(&win_state);

	while (!engine.quit) {
		window::pump_messages(&win_state);
		dx12::update();
		dx12::render();
	}

	dx12::shutdown();
	window::shutdown(&win_state);

	return 0;
}
