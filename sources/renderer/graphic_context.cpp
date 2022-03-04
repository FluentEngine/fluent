#include "core/window.hpp"
#include "core/application.hpp"
#include "renderer/renderer_backend.hpp"
#include "renderer/graphic_context.hpp"

namespace fluent
{
static GraphicContext* graphic_context = nullptr;

GraphicContext::GraphicContext(const GraphicContextDesc& desc)
{
    m_width  = desc.width;
    m_height = desc.height;

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

	create_command_buffers(
	    m_device, m_command_pool, FRAME_COUNT, m_command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        create_semaphore(m_device, &m_image_available_semaphores[ i ]);
        create_semaphore(m_device, &m_rendering_finished_semaphores[ i ]);
        create_fence(m_device, &m_in_flight_fences[ i ]);
        m_command_buffers_recorded[ i ] = false;
    }

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width           = desc.width;
    swapchain_desc.height          = desc.height;
    swapchain_desc.queue           = m_queue;
    swapchain_desc.min_image_count = FRAME_COUNT;
    swapchain_desc.builtin_depth   = desc.builtin_depth;

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
	destroy_command_buffers(
	    m_device, m_command_pool, FRAME_COUNT, m_command_buffers);
    destroy_command_pool(m_device, m_command_pool);
    destroy_queue(m_queue);
    destroy_device(m_device);
    destroy_renderer_backend(m_backend);
}

void GraphicContext::on_resize(u32 width, u32 height)
{
    queue_wait_idle(m_queue);
    resize_swapchain(m_device, m_swapchain, width, height);
    m_width  = width;
    m_height = height;
}

void GraphicContext::init(const GraphicContextDesc& desc)
{
    graphic_context = new GraphicContext(desc);
}

void GraphicContext::shutdown()
{
    delete graphic_context;
}

void GraphicContext::begin_frame()
{
    if (!m_command_buffers_recorded[ m_frame_index ])
    {
		wait_for_fences(m_device, 1, &m_in_flight_fences[ m_frame_index ]);
		reset_fences(m_device, 1, &m_in_flight_fences[ m_frame_index ]);
        m_command_buffers_recorded[ m_frame_index ] = true;
    }

	acquire_next_image(
	    m_device,
	    m_swapchain,
	    m_image_available_semaphores[ m_frame_index ],
	    nullptr,
	    &m_image_index);
    begin_command_buffer(acquire_cmd());
}

void GraphicContext::end_frame()
{
    end_command_buffer(acquire_cmd());

    QueueSubmitDesc queue_submit_desc{};
	queue_submit_desc.wait_semaphore_count = 1;
	queue_submit_desc.wait_semaphores =
	    m_image_available_semaphores[ m_frame_index ];
    queue_submit_desc.command_buffer_count   = 1;
    queue_submit_desc.command_buffers        = acquire_cmd();
    queue_submit_desc.signal_semaphore_count = 1;
	queue_submit_desc.signal_semaphores =
	    m_rendering_finished_semaphores[ m_frame_index ];
	queue_submit_desc.signal_fence = m_in_flight_fences[ m_frame_index ];

    queue_submit(m_queue, &queue_submit_desc);

    QueuePresentDesc queue_present_desc{};
    queue_present_desc.wait_semaphore_count = 1;
	queue_present_desc.wait_semaphores =
	    m_rendering_finished_semaphores[ m_frame_index ];
	queue_present_desc.swapchain   = m_swapchain;
	queue_present_desc.image_index = m_image_index;

    queue_present(m_queue, &queue_present_desc);

    m_command_buffers_recorded[ m_frame_index ] = false;
	m_frame_index = (m_frame_index + 1) % FRAME_COUNT;
}

void GraphicContext::begin_render_pass(f32 r, f32 g, f32 b, f32 a) const
{
    auto* cmd = acquire_cmd();

    ImageBarrier to_color_attachment{};
    to_color_attachment.src_queue = queue();
    to_color_attachment.dst_queue = queue();
    to_color_attachment.image     = acquire_image();
    to_color_attachment.old_state = ResourceState::eUndefined;
    to_color_attachment.new_state = ResourceState::eColorAttachment;

    cmd_barrier(cmd, 0, nullptr, 1, &to_color_attachment);

    RenderPassBeginDesc render_pass_begin_desc{};
	render_pass_begin_desc.render_pass =
	    get_swapchain_render_pass(m_swapchain, m_image_index);
    render_pass_begin_desc.clear_values[ 0 ].color[ 0 ] = r;
    render_pass_begin_desc.clear_values[ 0 ].color[ 1 ] = g;
    render_pass_begin_desc.clear_values[ 0 ].color[ 2 ] = b;
    render_pass_begin_desc.clear_values[ 0 ].color[ 3 ] = a;
    render_pass_begin_desc.clear_values[ 1 ].depth      = 1.0f;
    render_pass_begin_desc.clear_values[ 1 ].stencil    = 0;

    cmd_begin_render_pass(cmd, &render_pass_begin_desc);
}

void GraphicContext::end_render_pass() const
{
    auto* cmd = acquire_cmd();

    cmd_end_render_pass(cmd);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_queue = m_queue;
    to_present_barrier.dst_queue = m_queue;
    to_present_barrier.image     = acquire_image();
    to_present_barrier.old_state = ResourceState::eUndefined;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier(cmd, 0, nullptr, 1, &to_present_barrier);
}

CommandBuffer* GraphicContext::acquire_cmd() const
{
    return m_command_buffers[ m_frame_index ];
}

Image* GraphicContext::acquire_image() const
{
    return m_swapchain->images[ m_image_index ];
}

GraphicContext* GraphicContext::get()
{
    return graphic_context;
}
} // namespace fluent
