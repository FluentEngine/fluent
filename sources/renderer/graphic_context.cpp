#include "core/window.hpp"
#include "core/application.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer/graphic_context.hpp"

namespace fluent
{
static GraphicContext* graphic_context = nullptr;

GraphicContext::GraphicContext()
{
    RendererBackendDesc context_desc{};
    context_desc.vulkan_allocator = nullptr;
    create_renderer_backend(&context_desc, &m_backend);

    DeviceDesc device_desc{};
    device_desc.frame_in_use_count = 2;
    create_device(m_backend, &device_desc, &m_device);

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;
    create_queue(m_device, &queue_desc, &m_queue);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = m_queue;
    create_command_pool(m_device, &command_pool_desc, &m_command_pool);

    create_command_buffers(m_device, m_command_pool, FRAME_COUNT, m_command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        create_semaphore(m_device, &m_image_available_semaphores[ i ]);
        create_semaphore(m_device, &m_rendering_finished_semaphores[ i ]);
        create_fence(m_device, &m_in_flight_fences[ i ]);
        m_command_buffers_recorded[ i ] = false;
    }

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width           = window_get_width(get_app_window());
    swapchain_desc.height          = window_get_height(get_app_window());
    swapchain_desc.queue           = m_queue;
    swapchain_desc.min_image_count = FRAME_COUNT;

    create_swapchain(m_device, &swapchain_desc, &m_swapchain);
}

GraphicContext::~GraphicContext()
{
    device_wait_idle(m_device);
    destroy_swapchain(m_device, m_swapchain);
    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        destroy_semaphore(m_device, m_image_available_semaphores[ i ]);
        destroy_semaphore(m_device, m_rendering_finished_semaphores[ i ]);
        destroy_fence(m_device, m_in_flight_fences[ i ]);
    }
    destroy_command_buffers(m_device, m_command_pool, FRAME_COUNT, m_command_buffers);
    destroy_command_pool(m_device, m_command_pool);
    destroy_queue(m_queue);
    destroy_device(m_device);
    destroy_renderer_backend(m_backend);
}

void GraphicContext::on_resize(u32 width, u32 height)
{
    queue_wait_idle(m_queue);
    resize_swapchain(m_device, m_swapchain, width, height);
}

void GraphicContext::init()
{
    graphic_context = new GraphicContext();
}

void GraphicContext::shutdown()
{
    delete graphic_context;
}

void GraphicContext::begin_frame()
{
    if (!m_command_buffers_recorded[ m_frame_index ])
    {
        wait_for_fences(m_device, 1, m_in_flight_fences[ m_frame_index ]);
        reset_fences(m_device, 1, m_in_flight_fences[ m_frame_index ]);
        m_command_buffers_recorded[ m_frame_index ] = true;
    }

    acquire_next_image(m_device, m_swapchain, m_image_available_semaphores[ m_frame_index ], nullptr, &m_image_index);
}

void GraphicContext::end_frame()
{
    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.wait_semaphore_count   = 1;
    queue_submit_desc.wait_semaphores        = m_image_available_semaphores[ m_frame_index ];
    queue_submit_desc.command_buffer_count   = 1;
    queue_submit_desc.command_buffers        = acquire_cmd();
    queue_submit_desc.signal_semaphore_count = 1;
    queue_submit_desc.signal_semaphores      = m_rendering_finished_semaphores[ m_frame_index ];
    queue_submit_desc.signal_fence           = m_in_flight_fences[ m_frame_index ];

    queue_submit(m_queue, &queue_submit_desc);

    QueuePresentDesc queue_present_desc{};
    queue_present_desc.wait_semaphore_count = 1;
    queue_present_desc.wait_semaphores      = m_rendering_finished_semaphores[ m_frame_index ];
    queue_present_desc.swapchain            = m_swapchain;
    queue_present_desc.image_index          = m_image_index;

    queue_present(m_queue, &queue_present_desc);

    m_command_buffers_recorded[ m_frame_index ] = false;
    m_frame_index                               = (m_frame_index + 1) % FRAME_COUNT;
}

CommandBuffer* GraphicContext::acquire_cmd()
{
    return m_command_buffers[ m_frame_index ];
}

BaseImage* GraphicContext::acquire_image()
{
    return m_swapchain->images[ m_image_index ];
}

GraphicContext* GraphicContext::get()
{
    return graphic_context;
}
} // namespace fluent
