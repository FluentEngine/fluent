#pragma once

#ifdef D3D12_BACKEND

#include <d3d12.h>
#include <dxgi1_6.h>
//#include <D3D12MemoryAllocator/D3D12MemAlloc.h>
#undef MemoryBarrier
#include "../renderer_backend.h"

#undef interface

struct D3D12RendererBackend
{
	ID3D12Debug*               debug_controller;
	IDXGIFactory4*             factory;
	struct ft_instance interface;
};

struct D3D12Device
{
	IDXGIFactory4* factory;
	IDXGIAdapter*  adapter;
	ID3D12Device*  device;
	// D3D12MA::Allocator*   allocator;
	ID3D12DescriptorHeap* cbv_srv_uav_heap;
	uint64_t              cbv_srv_uav_descriptor_size;
	ID3D12DescriptorHeap* rtv_heap;
	uint64_t              rtv_descriptor_size;
	ID3D12DescriptorHeap* dsv_heap;
	uint64_t              dsv_descriptor_size;
	ID3D12DescriptorHeap* sampler_heap;
	uint64_t              sampler_descriptor_size;
	struct ft_device      interface;
};

struct D3D12CommandPool
{
	ID3D12CommandAllocator* command_allocator;
	struct ft_command_pool  interface;
};

struct D3D12CommandBuffer
{
	ID3D12CommandAllocator*    command_allocator;
	ID3D12GraphicsCommandList* command_list;
	struct ft_command_buffer   interface;
};

struct D3D12Queue
{
	ID3D12CommandQueue* queue;
	ID3D12Fence*        fence;
	struct ft_queue     interface;
};

struct D3D12Semaphore
{
	ID3D12Fence*        fence;
	uint64_t            fence_value;
	struct ft_semaphore interface;
};

struct D3D12Fence
{
	ID3D12Fence*    fence;
	HANDLE          event_handle;
	uint64_t        fence_value;
	struct ft_fence interface;
};

struct D3D12Sampler
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	struct ft_sampler           interface;
};

struct D3D12Image
{
	ID3D12Resource*             image;
	D3D12_CPU_DESCRIPTOR_HANDLE image_view;
	// D3D12MA::Allocation*        allocation;
	struct ft_image interface;
};

struct D3D12Buffer
{
	ID3D12Resource* buffer;
	// D3D12MA::Allocation* allocation;
	struct ft_buffer interface;
};

struct D3D12Swapchain
{
	IDXGISwapChain*     swapchain;
	uint32_t            image_index;
	struct ft_swapchain interface;
};

struct D3D12Shader
{
	D3D12_SHADER_BYTECODE
	bytecodes[ FT_SHADER_STAGE_COUNT ];
	struct ft_shader interface;
};

struct D3D12DescriptorSetLayout
{
	ID3D12RootSignature*            root_signature;
	struct ft_descriptor_set_layout interface;
};

struct D3D12Pipeline
{
	ID3D12PipelineState*     pipeline;
	ID3D12RootSignature*     root_signature;
	D3D12_PRIMITIVE_TOPOLOGY topology;
	struct ft_pipeline       interface;
};

struct D3D12DescriptorSet
{
	struct ft_descriptor_set interface;
};

struct D3D12UiContext
{
	ID3D12DescriptorHeap* cbv_srv_heap;
};

void
d3d12_create_renderer_backend( const struct ft_instance_info* info,
                               struct ft_instance**           backend );

#endif
