#pragma once

#include "window.h"

#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <directx/d3dx12.h>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")

struct dx12_context {
	static const UINT frame_count = 2;

	// Pipeline objects
#if defined(_DEBUG)
	ID3D12Debug* debug_controller;
#endif
	IDXGIFactory4* factory;
	ID3D12Device* device;
	ID3D12CommandQueue* graphics_command_queue;
	IDXGISwapChain4* swapchain;
	ID3D12DescriptorHeap* rtv_descriptor_heap;
	ID3D12Resource* render_targets[frame_count];
	ID3D12CommandAllocator* command_allocator;
	ID3D12GraphicsCommandList* command_list;

	UINT rtv_descriptor_heap_size;

	// Synchronization objects
	ID3D12Fence* fence;
	UINT64 fence_value;
	HANDLE fence_event;
	UINT frame_index;
};

namespace dx12 {
	void initialize(window_state* win_state);
	void shutdown();
	void update();
	void render();
}