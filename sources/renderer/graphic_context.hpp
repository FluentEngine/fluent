#pragma once

#include "renderer/renderer_backend.hpp"

namespace fluent
{
struct GraphicContextDesc
{
    u32 width         = 0;
    u32 height        = 0;
    b32 builtin_depth = false;
};

class GraphicContext
{
private:
    static constexpr u32 FRAME_COUNT = 2;

    RendererBackend* m_backend;
    Device* m_device;
    Queue* m_queue;
    CommandPool* m_command_pool;
    Semaphore* m_image_available_semaphores[ FRAME_COUNT ];
    Semaphore* m_rendering_finished_semaphores[ FRAME_COUNT ];
    Fence* m_in_flight_fences[ FRAME_COUNT ];
    bool m_command_buffers_recorded[ FRAME_COUNT ];

    Swapchain* m_swapchain;
    CommandBuffer* m_command_buffers[ FRAME_COUNT ];

    u32 m_frame_index = 0;
    u32 m_image_index = 0;

    u32 m_width  = 0;
    u32 m_height = 0;

    Image* acquire_image() const;

    GraphicContext( const GraphicContextDesc& desc );

public:
    ~GraphicContext();

    void on_resize( u32 width, u32 height );
    void begin_frame();
    void end_frame();

    CommandBuffer* acquire_cmd() const;

    RendererBackend* backend() const { return m_backend; }

    Device* device() const { return m_device; }

    Queue* queue() const { return m_queue; }

    Swapchain* swapchain() const { return m_swapchain; }

    u32 width() const { return m_width; }

    u32 height() const { return m_height; }

    void begin_render_pass( f32 r, f32 g, f32 b, f32 a ) const;
    void end_render_pass() const;

    u32 frame_index() const { return m_frame_index; }

    static void init( const GraphicContextDesc& desc );
    static void shutdown();

    static GraphicContext* get();

    static constexpr u32 frame_count() { return FRAME_COUNT; }
};
} // namespace fluent
