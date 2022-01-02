#pragma once

#include "volk.h"
#include "core/base.hpp"
#include "renderer/renderer_enums.hpp"

namespace fluent
{

static constexpr u32 MAX_ATTACHMENTS_COUNT = 10;

struct RendererDescription
{
    VkAllocationCallbacks* vulkan_allocator;
};

enum class QueueType : u8
{
    eGraphics = 0,
    eCompute = 1,
    eTransfer = 2,
    eLast
};

struct Renderer
{
    VkAllocationCallbacks* m_vulkan_allocator;
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debug_messenger;
    VkPhysicalDevice m_physical_device;
};

struct DeviceDescription
{
};

struct Device
{
    VkAllocationCallbacks* m_vulkan_allocator;
    VkInstance m_instance;
    VkPhysicalDevice m_physical_device;
    VkDevice m_logical_device;
};

struct QueueDescription
{
    QueueType queue_type;
};

struct Queue
{
    u32 m_family_index;
    VkQueue m_queue;
};

struct Semaphore
{
    VkSemaphore m_semaphore;
};

struct Fence
{
    VkFence m_fence = VK_NULL_HANDLE;
};

struct SwapchainDescription
{
    Queue* queue;
    u32 width;
    u32 height;
    u32 image_count;
};

struct Image
{
    VkImage m_image;
    VkImageView m_image_view;
    u32 m_width;
    u32 m_height;
    Format m_format;
    SampleCount m_sample_count;
};

struct Swapchain
{
    VkPresentModeKHR m_present_mode;
    u32 m_image_count;
    u32 m_width;
    u32 m_height;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    Format m_format;
    Image* m_images;
    u32 m_current_image_index;
};

struct CommandPoolDescription
{
    Queue* queue;
};

struct CommandPool
{
    VkCommandPool m_command_pool;
};

struct CommandBuffer
{
    VkCommandBuffer m_command_buffer;
};

struct QueueSubmitDescription
{
    u32 wait_semaphore_count;
    Semaphore* wait_semaphores;
    u32 command_buffer_count;
    CommandBuffer* command_buffers;
    u32 signal_semaphore_count;
    Semaphore* signal_semaphores;
    Fence* signal_fence;
};

struct QueuePresentDescription
{
    u32 wait_semaphore_count;
    Semaphore* wait_semaphores;
    Swapchain* swapchain;
    u32 image_index;
};

struct ClearValue
{
    f32 color[ 4 ];
};

struct RenderPassInfo
{
    u32 color_attachment_count;
    Image* color_attachments[ MAX_ATTACHMENTS_COUNT ];
    AttachmentLoadOp color_attachment_load_ops[ MAX_ATTACHMENTS_COUNT ];
    ClearValue color_clear_values[ MAX_ATTACHMENTS_COUNT ];
    Image* depth_stencil;
    f32 depth_clear_value;
    u32 stencil_clear_value;
    AttachmentLoadOp depth_stencil_load_op;
    u32 width;
    u32 height;
};

Renderer create_renderer(const RendererDescription& description);
void destroy_renderer(Renderer& renderer);

Device create_device(const Renderer& renderer, const DeviceDescription& description);
void destroy_device(Device& device);
void device_wait_idle(const Device& device);

Queue get_queue(const Device& device, const QueueDescription& description);
void queue_wait_idle(const Queue& queue);
void queue_submit(const Queue& queue, const QueueSubmitDescription& description);
void queue_present(const Queue& queue, const QueuePresentDescription& description);

Semaphore create_semaphore(const Device& device);
void destroy_semaphore(const Device& device, Semaphore& semaphore);
Fence create_fence(const Device& device);
void destroy_fence(const Device& device, Fence& fence);
void wait_for_fences(const Device& device, u32 count, Fence* fences);
void reset_fences(const Device& device, u32 count, Fence* fences);

Swapchain create_swapchain(const Renderer& renderer, const Device& device, const SwapchainDescription& description);
void destroy_swapchain(const Device& device, Swapchain& swapchain);

CommandPool create_command_pool(const Device& device, const CommandPoolDescription& description);
void destroy_command_pool(const Device& device, CommandPool& command_pool);

void allocate_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void free_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers);
void begin_command_buffer(const CommandBuffer& command_buffer);
void end_command_buffer(const CommandBuffer& command_buffer);

void acquire_next_image(
    const Device& device, const Swapchain& swapchain, const Semaphore& semaphore, const Fence& fence, u32& image_index);

void cmd_begin_render_pass(const Device& device, const CommandBuffer& command_buffer, const RenderPassInfo& info);
void cmd_end_render_pass(const CommandBuffer& command_buffer);

} // namespace fluent