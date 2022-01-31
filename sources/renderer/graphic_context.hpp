#pragma once

#include "renderer/renderer_backend.hpp"

namespace fluent
{
class GraphicContext
{
private:
    static constexpr u32 FRAME_COUNT = 2;

    RendererBackend* m_backend;
    Device*          m_device;
    Queue*           m_queue;
    CommandPool*     m_command_pool;
    Semaphore*       m_image_available_semaphores[ FRAME_COUNT ];
    Semaphore*       m_rendering_finished_semaphores[ FRAME_COUNT ];
    Fence*           m_in_flight_fences[ FRAME_COUNT ];
    bool             m_command_buffers_recorded[ FRAME_COUNT ];

    Swapchain*     m_swapchain;
    CommandBuffer* m_command_buffers[ FRAME_COUNT ];

    u32 m_frame_index = 0;
    u32 m_image_index = 0;

    GraphicContext();

public:
    ~GraphicContext();

    void on_resize(u32 width, u32 height);
    void begin_frame();
    void end_frame();

    CommandBuffer* acquire_cmd();
    BaseImage*     acquire_image();

    RendererBackend* backend() const
    {
        return m_backend;
    }

    Device* device() const
    {
        return m_device;
    }

    Queue* queue() const
    {
        return m_queue;
    }

    Swapchain* swapchain() const
    {
        return m_swapchain;
    }

    const RenderPass* acquire_render_pass() const
    {
        return get_swapchain_render_pass(m_swapchain, m_image_index);
    }

    static void init();
    static void shutdown();

    static GraphicContext* get();
};
} // namespace fluent
