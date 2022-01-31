#include "core/application.hpp"
#include "utils/file_loader.hpp"
#include "renderer/renderer_3d.hpp"

namespace fluent
{
GraphicContext*          Renderer3D::m_context = nullptr;
Renderer3D::RendererData Renderer3D::m_data    = {};

void Renderer3D::setup_vertex_layout(VertexLayout& vertex_layout)
{
    vertex_layout.binding_desc_count            = 1;
    vertex_layout.binding_descs[ 0 ].binding    = 0;
    vertex_layout.binding_descs[ 0 ].input_rate = VertexInputRate::eVertex;
    vertex_layout.binding_descs[ 0 ].stride     = 14 * sizeof(f32);
    vertex_layout.attribute_desc_count          = 5;
    vertex_layout.attribute_descs[ 0 ].binding  = 0;
    vertex_layout.attribute_descs[ 0 ].format   = Format::eR32G32B32Sfloat; // Positions
    vertex_layout.attribute_descs[ 0 ].location = 0;
    vertex_layout.attribute_descs[ 0 ].offset   = 0;
    vertex_layout.attribute_descs[ 1 ].binding  = 0;
    vertex_layout.attribute_descs[ 1 ].format   = Format::eR32G32B32Sfloat; // Normals
    vertex_layout.attribute_descs[ 1 ].location = 1;
    vertex_layout.attribute_descs[ 1 ].offset   = 3 * sizeof(f32);
    vertex_layout.attribute_descs[ 2 ].binding  = 0;
    vertex_layout.attribute_descs[ 2 ].format   = Format::eR32G32Sfloat; // UVs
    vertex_layout.attribute_descs[ 2 ].location = 2;
    vertex_layout.attribute_descs[ 2 ].offset   = 6 * sizeof(f32);
    vertex_layout.attribute_descs[ 3 ].binding  = 0;
    vertex_layout.attribute_descs[ 3 ].format   = Format::eR32G32B32Sfloat; // Tangents
    vertex_layout.attribute_descs[ 3 ].location = 3;
    vertex_layout.attribute_descs[ 3 ].offset   = 8 * sizeof(f32);
    vertex_layout.attribute_descs[ 4 ].binding  = 0;
    vertex_layout.attribute_descs[ 4 ].format   = Format::eR32G32B32Sfloat; // Bitangents
    vertex_layout.attribute_descs[ 4 ].location = 4;
    vertex_layout.attribute_descs[ 4 ].offset   = 11 * sizeof(f32);
}

void Renderer3D::init(u32 width, u32 height)
{
    m_context              = GraphicContext::get();
    m_data.viewport_width  = width;
    m_data.viewport_height = height;
    create_output_images(width, height);
    create_render_pass(width, height);

    auto vert_code = read_file_binary(get_app_shaders_directory() + "/main.vert.glsl.spv");
    auto frag_code = read_file_binary(get_app_shaders_directory() + "/main.frag.glsl.spv");

    ShaderDesc shader_descs[ 2 ];
    shader_descs[ 0 ].stage         = ShaderStage::eVertex;
    shader_descs[ 0 ].bytecode_size = vert_code.size() * sizeof(vert_code[ 0 ]);
    shader_descs[ 0 ].bytecode      = vert_code.data();
    shader_descs[ 1 ].stage         = ShaderStage::eFragment;
    shader_descs[ 1 ].bytecode_size = frag_code.size() * sizeof(frag_code[ 0 ]);
    shader_descs[ 1 ].bytecode      = frag_code.data();

    Shader* shaders[ 2 ] = {};
    create_shader(GraphicContext::get()->device(), &shader_descs[ 0 ], &shaders[ 0 ]);
    create_shader(GraphicContext::get()->device(), &shader_descs[ 1 ], &shaders[ 1 ]);

    create_descriptor_set_layout(GraphicContext::get()->device(), 2, shaders, &m_data.descriptor_set_layout);

    PipelineDesc pipeline_desc{};
    setup_vertex_layout(pipeline_desc.vertex_layout);
    pipeline_desc.rasterizer_desc.cull_mode    = CullMode::eBack;
    pipeline_desc.rasterizer_desc.front_face   = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test  = true;
    pipeline_desc.depth_state_desc.depth_write = true;
    pipeline_desc.depth_state_desc.compare_op  = CompareOp::eLess;
    pipeline_desc.descriptor_set_layout        = m_data.descriptor_set_layout;
    pipeline_desc.render_pass                  = m_data.render_pass;

    create_graphics_pipeline(GraphicContext::get()->device(), &pipeline_desc, &m_data.pipeline);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(GraphicContext::get()->device(), shaders[ i ]);
    }

    BufferLoadDesc ubo_load_desc{};
    ubo_load_desc.size          = sizeof(CameraData);
    BufferDesc& buffer_desc     = ubo_load_desc.buffer_desc;
    buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;
    buffer_desc.size            = ubo_load_desc.size;

    ResourceManager::load_buffer(m_data.uniform_buffer, &ubo_load_desc);

    DescriptorSetDesc set_desc{};
    set_desc.descriptor_set_layout = m_data.descriptor_set_layout;
    create_descriptor_set(GraphicContext::get()->device(), &set_desc, &m_data.set);

    DescriptorBufferDesc set_buffer_desc{};
    set_buffer_desc.offset = 0;
    set_buffer_desc.range  = sizeof(CameraData);
    set_buffer_desc.buffer = m_data.uniform_buffer->get();

    DescriptorWriteDesc set_write{};
    set_write.binding                 = 0;
    set_write.descriptor_type         = DescriptorType::eUniformBuffer;
    set_write.descriptor_count        = 1;
    set_write.descriptor_buffer_descs = &set_buffer_desc;

    update_descriptor_set(GraphicContext::get()->device(), m_data.set, 1, &set_write);
}

void Renderer3D::shutdown()
{
    device_wait_idle(m_context->device());
    destroy_descriptor_set(m_context->device(), m_data.set);
    ResourceManager::release_buffer(m_data.uniform_buffer);
    destroy_descriptor_set_layout(m_context->device(), m_data.descriptor_set_layout);
    destroy_pipeline(m_context->device(), m_data.pipeline);
    destroy_render_pass(m_context->device(), m_data.render_pass);
    destroy_output_images();
}

void Renderer3D::create_output_images(u32 width, u32 height)
{
    ImageDesc output_image_descs[ 2 ]       = {};
    output_image_descs[ 0 ].width           = width;
    output_image_descs[ 0 ].height          = height;
    output_image_descs[ 0 ].depth           = 1.0f;
    output_image_descs[ 0 ].format          = Format::eR8G8B8A8Unorm;
    output_image_descs[ 0 ].layer_count     = 1;
    output_image_descs[ 0 ].mip_levels      = 1;
    output_image_descs[ 0 ].sample_count    = SampleCount::e1;
    output_image_descs[ 0 ].descriptor_type = DescriptorType::eColorAttachment | DescriptorType::eSampledImage;

    output_image_descs[ 1 ].width           = width;
    output_image_descs[ 1 ].height          = height;
    output_image_descs[ 1 ].depth           = 1.0f;
    output_image_descs[ 1 ].format          = Format::eD32Sfloat;
    output_image_descs[ 1 ].layer_count     = 1;
    output_image_descs[ 1 ].mip_levels      = 1;
    output_image_descs[ 1 ].sample_count    = SampleCount::e1;
    output_image_descs[ 1 ].descriptor_type = DescriptorType::eDepthStencilAttachment;

    create_image(m_context->device(), &output_image_descs[ 0 ], &m_data.output_image);
    create_image(m_context->device(), &output_image_descs[ 1 ], &m_data.output_depth_image);

    ImageBarrier image_barriers[ 2 ] = {};
    image_barriers[ 0 ].image        = m_data.output_image;
    image_barriers[ 0 ].src_queue    = m_context->queue();
    image_barriers[ 0 ].dst_queue    = m_context->queue();
    image_barriers[ 0 ].old_state    = ResourceState::eUndefined;
    image_barriers[ 0 ].new_state    = ResourceState::eColorAttachment;
    image_barriers[ 1 ].image        = m_data.output_depth_image;
    image_barriers[ 1 ].src_queue    = m_context->queue();
    image_barriers[ 1 ].dst_queue    = m_context->queue();
    image_barriers[ 1 ].old_state    = ResourceState::eUndefined;
    image_barriers[ 1 ].new_state    = ResourceState::eDepthStencilWrite;

    auto* cmd = m_context->acquire_cmd();
    begin_command_buffer(cmd);
    cmd_barrier(cmd, 0, nullptr, 2, image_barriers);
    end_command_buffer(cmd);
    immediate_submit(m_context->queue(), cmd);
    queue_wait_idle(m_context->queue());
}

void Renderer3D::destroy_output_images()
{
    destroy_image(m_context->device(), m_data.output_image);
    destroy_image(m_context->device(), m_data.output_depth_image);
}

void Renderer3D::create_render_pass(u32 width, u32 height)
{
    RenderPassDesc rp_desc{};
    rp_desc.width                          = width;
    rp_desc.height                         = height;
    rp_desc.color_attachment_count         = 1;
    rp_desc.color_attachments[ 0 ]         = m_data.output_image;
    rp_desc.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    rp_desc.color_image_states[ 0 ]        = ResourceState::eColorAttachment;
    rp_desc.depth_stencil                  = m_data.output_depth_image;
    rp_desc.depth_stencil_load_op          = AttachmentLoadOp::eClear;
    rp_desc.depth_stencil_state            = ResourceState::eDepthStencilWrite;

    fluent::create_render_pass(m_context->device(), &rp_desc, &m_data.render_pass);
}

void Renderer3D::begin_frame(Camera& camera)
{
    map_memory(m_context->device(), m_data.uniform_buffer->get());
    std::memcpy(m_data.uniform_buffer->get()->mapped_memory, &camera.get_data(), sizeof(CameraData));
    unmap_memory(m_context->device(), m_data.uniform_buffer->get());

    ImageBarrier to_color_attachment{};
    to_color_attachment.image     = m_data.output_image;
    to_color_attachment.old_state = ResourceState::eUndefined;
    to_color_attachment.new_state = ResourceState::eColorAttachment;
    to_color_attachment.src_queue = m_context->queue();
    to_color_attachment.dst_queue = m_context->queue();

    auto* cmd = m_context->acquire_cmd();
    cmd_barrier(cmd, 0, nullptr, 1, &to_color_attachment);

    RenderPassBeginDesc rp_begin_desc{};
    rp_begin_desc.clear_values[ 0 ].color[ 0 ] = 0.5;
    rp_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.6;
    rp_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4;
    rp_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0;
    rp_begin_desc.clear_values[ 1 ].depth      = 1.0f;
    rp_begin_desc.clear_values[ 1 ].stencil    = 0;
    rp_begin_desc.render_pass                  = m_data.render_pass;

    cmd_begin_render_pass(cmd, &rp_begin_desc);

    cmd_set_viewport(cmd, 0, 0, m_data.viewport_width, m_data.viewport_height, 0.0f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, m_data.viewport_width, m_data.viewport_height);
    cmd_bind_pipeline(cmd, m_data.pipeline);
    cmd_bind_descriptor_set(cmd, m_data.set, m_data.pipeline);
}

void Renderer3D::draw_geometry(const Matrix4& transform, const Ref<Geometry>& geometry)
{
    auto* cmd = m_context->acquire_cmd();
    for (auto& node : geometry->nodes())
    {
        cmd_push_constants(cmd, m_data.pipeline, 0, sizeof(Matrix4), &transform);
        cmd_bind_vertex_buffer(cmd, node.vertex_buffer->get());
        cmd_bind_index_buffer_u32(cmd, node.index_buffer->get());
        cmd_draw_indexed(cmd, node.index_count, 1, 0, 0, 0);
    }
}

void Renderer3D::end_frame()
{
    auto* cmd = m_context->acquire_cmd();
    cmd_end_render_pass(cmd);

    ImageBarrier to_sampled{};
    to_sampled.image     = m_data.output_image;
    to_sampled.old_state = ResourceState::eColorAttachment;
    to_sampled.new_state = ResourceState::eShaderReadOnly;
    to_sampled.src_queue = m_context->queue();
    to_sampled.dst_queue = m_context->queue();

    cmd_barrier(cmd, 0, nullptr, 1, &to_sampled);
}

void Renderer3D::resize_viewport(u32 width, u32 height)
{
    m_data.viewport_width  = width;
    m_data.viewport_height = height;

    destroy_output_images();
    create_output_images(m_data.viewport_width, m_data.viewport_height);
    RenderPassDesc rp_desc{};
    rp_desc.width                          = m_data.viewport_width;
    rp_desc.height                         = m_data.viewport_height;
    rp_desc.color_attachment_count         = 1;
    rp_desc.color_attachments[ 0 ]         = m_data.output_image;
    rp_desc.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    rp_desc.color_image_states[ 0 ]        = ResourceState::eColorAttachment;
    rp_desc.depth_stencil                  = m_data.output_depth_image;
    rp_desc.depth_stencil_load_op          = AttachmentLoadOp::eClear;
    rp_desc.depth_stencil_state            = ResourceState::eDepthStencilWrite;
    update_render_pass(m_context->device(), m_data.render_pass, &rp_desc);
}
} // namespace fluent
