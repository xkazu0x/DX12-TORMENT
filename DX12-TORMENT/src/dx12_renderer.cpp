#include "dx12_renderer.h"

#define CHECK(x) \
	if (!(x)) { MessageBoxA(0, #x, "CHECK FAILED", MB_ICONEXCLAMATION | MB_OK); __debugbreak(); }

#define DXCHECK(expr) \
	CHECK(SUCCEEDED(expr));

static dx12_context context;

void pipeline_create(window_state* win_state);
void load_assets();
void populate_command_list();
void wait_for_previous_frame();

void dx12::initialize(window_state* win_state) {
	pipeline_create(win_state);
	load_assets();
}

void dx12::shutdown() {
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	wait_for_previous_frame();
	CloseHandle(context.fence_event);
}

void dx12::update() {
}

void dx12::render() {
	// Record all the commands we need to render the scene into the command list.
	populate_command_list();

	// Execute the command list.
	ID3D12CommandList* command_lists[] = { context.command_list };
	context.graphics_command_queue->ExecuteCommandLists(_countof(command_lists), command_lists);

	// Present the frame.
	DXCHECK(context.swapchain->Present(1, 0));

	wait_for_previous_frame();
}

void pipeline_create(window_state* win_state) {
	UINT dxgi_factory_flags = 0;
#if defined(_DEBUG)
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&context.debug_controller)))) {
		context.debug_controller->EnableDebugLayer();
		dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	DXCHECK(CreateDXGIFactory2(
		dxgi_factory_flags,
		IID_PPV_ARGS(&context.factory)));

	DXCHECK(D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&context.device)));

	// Command Queue
	D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	command_queue_desc.NodeMask = 0;

	DXCHECK(context.device->CreateCommandQueue(
		&command_queue_desc,
		IID_PPV_ARGS(&context.graphics_command_queue)));

	// Swapchain
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.Width = static_cast<UINT>(win_state->width);
	swap_chain_desc.Height = static_cast<UINT>(win_state->height);
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.Stereo = FALSE;
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = context.frame_count;
	swap_chain_desc.Scaling = DXGI_SCALING_STRETCH;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	swap_chain_desc.Flags = 0;

	IDXGISwapChain1* swapchain1;
	DXCHECK(context.factory->CreateSwapChainForHwnd(
		context.graphics_command_queue,
		win_state->hwnd,
		&swap_chain_desc,
		nullptr,
		nullptr,
		&swapchain1));

	context.swapchain = static_cast<IDXGISwapChain4*>(swapchain1);
	context.frame_index = context.swapchain->GetCurrentBackBufferIndex();

	// Descriptor Heap
	D3D12_DESCRIPTOR_HEAP_DESC rtv_descriptor_heap_desc = {};
	rtv_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_descriptor_heap_desc.NumDescriptors = context.frame_count;
	rtv_descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_descriptor_heap_desc.NodeMask = 0;

	DXCHECK(context.device->CreateDescriptorHeap(
		&rtv_descriptor_heap_desc,
		IID_PPV_ARGS(&context.rtv_descriptor_heap)));

	context.rtv_descriptor_heap_size =
		context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create frame resources
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
		context.rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());

	// Create a rtv descriptor for each frame.
	for (int i = 0; i < context.frame_count; i++) {
		DXCHECK(context.swapchain->GetBuffer(i, IID_PPV_ARGS(&context.render_targets[i])));
		context.device->CreateRenderTargetView(context.render_targets[i], nullptr, rtv_handle);
		rtv_handle.Offset(1, context.rtv_descriptor_heap_size);
	}

	DXCHECK(context.device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&context.command_allocator)));
}

void load_assets() {
	// Create command list
	DXCHECK(context.device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		context.command_allocator,
		nullptr,
		IID_PPV_ARGS(&context.command_list)));

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	DXCHECK(context.command_list->Close());

	// Create synchronization objects
	DXCHECK(context.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&context.fence)));
	context.fence_value = 1;

	// Create an event handle to use for frame synchronization
	context.fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (context.fence_event == nullptr) {
		HRESULT_FROM_WIN32(GetLastError());
	}
}

void populate_command_list() {
	DXCHECK(context.command_allocator->Reset());
	DXCHECK(context.command_list->Reset(context.command_allocator, nullptr));

	// Indicate that the back buffer will be used as a render target.
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			context.render_targets[context.frame_index],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		context.command_list->ResourceBarrier(1, &barrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
			context.rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
			context.frame_index,
			context.rtv_descriptor_heap_size);

		// Record commands.
		const float clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		context.command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
	}

	// Indicate that the back buffer will now be used to present.
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			context.render_targets[context.frame_index],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		context.command_list->ResourceBarrier(
			1, &barrier);

		DXCHECK(context.command_list->Close());
	}
}

void wait_for_previous_frame() {
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence_value = context.fence_value;
	DXCHECK(context.graphics_command_queue->Signal(context.fence, fence_value));
	context.fence_value++;

	// Wait until the previous frame is finished.
	if (context.fence->GetCompletedValue() < fence_value) {
		DXCHECK(context.fence->SetEventOnCompletion(fence_value, context.fence_event));
		WaitForSingleObject(context.fence_event, INFINITE);
	}

	context.frame_index = context.swapchain->GetCurrentBackBufferIndex();
}
