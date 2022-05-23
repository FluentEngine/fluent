#pragma once

#ifdef D3D12_BACKEND

#define INCLUDE_WIN_DEFINES
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3D12MemAlloc.h>
#undef MemoryBarrier
#include "renderer/renderer_backend.hpp"

#undef interface

namespace fluent
{
struct D3D12RendererBackend
{
	ID3D12Debug*    debug_controller;
	IDXGIFactory4*  factory;
	RendererBackend interface;
};

struct D3D12Device
{
	IDXGIFactory4*        factory;
	IDXGIAdapter*         adapter;
	ID3D12Device*         device;
	D3D12MA::Allocator*   allocator;
	ID3D12DescriptorHeap* cbv_srv_uav_heap;
	u64                   cbv_srv_uav_descriptor_size;
	ID3D12DescriptorHeap* rtv_heap;
	u64                   rtv_descriptor_size;
	ID3D12DescriptorHeap* dsv_heap;
	u64                   dsv_descriptor_size;
	ID3D12DescriptorHeap* sampler_heap;
	u64                   sampler_descriptor_size;
	Device                interface;
};

struct D3D12CommandPool
{
	ID3D12CommandAllocator* command_allocator;
	CommandPool             interface;
};

struct D3D12CommandBuffer
{
	ID3D12CommandAllocator*    command_allocator;
	ID3D12GraphicsCommandList* command_list;
	CommandBuffer              interface;
};

struct D3D12Queue
{
	ID3D12CommandQueue* queue;
	ID3D12Fence*        fence;
	Queue               interface;
};

struct D3D12Semaphore
{
	ID3D12Fence* fence;
	u64          fence_value;
	Semaphore    interface;
};

struct D3D12Fence
{
	ID3D12Fence* fence;
	HANDLE       event_handle;
	u64          fence_value;
	Fence        interface;
};

struct D3D12Sampler
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	Sampler                     interface;
};

struct D3D12Image
{
	ID3D12Resource*             image;
	D3D12_CPU_DESCRIPTOR_HANDLE image_view;
	D3D12MA::Allocation*        allocation;
	Image                       interface;
};

struct D3D12Buffer
{
	ID3D12Resource*      buffer;
	D3D12MA::Allocation* allocation;
	Buffer               interface;
};

struct D3D12Swapchain
{
	IDXGISwapChain* swapchain;
	mutable u32     image_index;
	Swapchain       interface;
};

struct D3D12Shader
{
	D3D12_SHADER_BYTECODE
	bytecodes[ static_cast<u32>( FT_SHADER_STAGE_COUNT ) ];
	Shader interface;
};

struct D3D12DescriptorSetLayout
{
	ID3D12RootSignature* root_signature;
	DescriptorSetLayout  interface;
};

struct D3D12Pipeline
{
	ID3D12PipelineState*     pipeline;
	ID3D12RootSignature*     root_signature;
	D3D12_PRIMITIVE_TOPOLOGY topology;
	Pipeline                 interface;
};

struct D3D12DescriptorSet
{
	DescriptorSet interface;
};

struct D3D12UiContext
{
	ID3D12DescriptorHeap* cbv_srv_heap;
};

void
d3d12_create_renderer_backend( const RendererBackendInfo* info,
                               struct RendererBackend**   backend );
} // namespace fluent
#endif
