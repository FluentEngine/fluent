#pragma once

#ifdef D3D12_BACKEND
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3D12MemAlloc.h>
#undef MemoryBarrier
#include "renderer/renderer_backend.hpp"

namespace fluent
{

struct D3D12RendererBackend
{
    ID3D12Debug*   debug_controller;
    IDXGIFactory4* factory;
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
};

struct D3D12CommandPool
{
    ID3D12CommandAllocator* command_allocator;
};

struct D3D12CommandBuffer
{
    ID3D12CommandAllocator*    command_allocator;
    ID3D12GraphicsCommandList* command_list;
};

struct D3D12Queue
{
    ID3D12CommandQueue* queue;
    ID3D12Fence*        fence;
};

struct D3D12Semaphore
{
    ID3D12Fence* fence;
    u64          fence_value;
};

struct D3D12Fence
{
    ID3D12Fence* fence;
    HANDLE       event_handle;
    u64          fence_value;
};

struct D3D12Sampler
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
};

struct D3D12Image
{
    ID3D12Resource*             image;
    D3D12_CPU_DESCRIPTOR_HANDLE image_view;
    D3D12MA::Allocation*        allocation;
};

struct D3D12Buffer
{
    ID3D12Resource*      buffer;
    D3D12MA::Allocation* allocation;
};

struct Swapchain
{
    IDXGISwapChain* swapchain;
    mutable u32     image_index;
};

struct D3D12RenderPass
{
    D3D12_CPU_DESCRIPTOR_HANDLE color_attachments[ MAX_ATTACHMENTS_COUNT ];
    DXGI_FORMAT                 color_formats[ MAX_ATTACHMENTS_COUNT ];
    D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil;
    DXGI_FORMAT                 depth_format;
};

struct D3D12Shader
{
    D3D12_SHADER_BYTECODE bytecode;
};

struct D3D12DescriptorSetLayout
{
    ID3D12RootSignature* root_signature;
};

struct D3D12Pipeline
{
    ID3D12PipelineState*     pipeline;
    ID3D12RootSignature*     root_signature;
    D3D12_PRIMITIVE_TOPOLOGY topology;
};

struct D3D12DescriptorSet
{
    VkDescriptorSet descriptor_set;
};

struct D3D12UiContext
{
    ID3D12DescriptorHeap* cbv_srv_heap;
};

void d3d12_create_renderer_backend( const RendererBackendDesc* desc,
                                    RendererBackend**          p_backend );
} // namespace fluent
#endif
