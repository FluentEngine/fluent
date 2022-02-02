#include "core/application.hpp"
#include "utils/file_loader.hpp"
#include "renderer/renderer_3d.hpp"

namespace fluent
{
GraphicContext*               Renderer3D::m_context             = nullptr;
Renderer3D::RendererData      Renderer3D::m_data                = {};
Renderer3D::RendererResources Renderer3D::m_resources           = {};
Renderer3D::FrameData         Renderer3D::m_frame_data          = {};
Renderer3D::PushConstantBlock Renderer3D::m_push_constant_block = {};

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

void Renderer3D::create_default_resources()
{
    u32 image_width  = 2;
    u32 image_height = 2;

    struct Color
    {
        u8 data[ 4 ];
    };

    Color magenta = { 255, 0, 255, 255 };

    std::vector<Color> magenta_image_data(image_width * image_height, magenta);

    ImageLoadDesc image_load_desc{};
    image_load_desc.size       = magenta_image_data.size() * sizeof(magenta_image_data[ 0 ]);
    image_load_desc.data       = magenta_image_data.data();
    ImageDesc& image_desc      = image_load_desc.image_desc;
    image_desc.width           = image_width;
    image_desc.height          = image_height;
    image_desc.depth           = 1;
    image_desc.format          = Format::eR8G8B8A8Unorm;
    image_desc.layer_count     = 1;
    image_desc.mip_levels      = 1;
    image_desc.sample_count    = SampleCount::e1;
    image_desc.descriptor_type = DescriptorType::eSampledImage;

    ResourceManager::load_image(m_resources.default_albedo, &image_load_desc);

    SamplerDesc sampler_desc{};
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.min_lod     = 0;
    sampler_desc.max_lod     = 16;
    create_sampler(m_context->device(), &sampler_desc, &m_resources.default_sampler);
}

void Renderer3D::destroy_default_resources()
{
    destroy_sampler(m_context->device(), m_resources.default_sampler);
    ResourceManager::release_image(m_resources.default_albedo);
}

void Renderer3D::init(u32 width, u32 height)
{
    m_context              = GraphicContext::get();
    m_data.viewport_width  = width;
    m_data.viewport_height = height;
    create_output_images(width, height);
    create_render_pass(width, height);

    auto vert_code       = read_file_binary(get_app_shaders_directory() + "/main.vert.glsl.spv");
    auto phong_frag_code = read_file_binary(get_app_shaders_directory() + "/phong.frag.glsl.spv");
    auto color_frag_code = read_file_binary(get_app_shaders_directory() + "/color.frag.glsl.spv");

    static constexpr u32 shader_count = 3;

    ShaderDesc shader_descs[ shader_count ] = {};
    shader_descs[ 0 ].stage                 = ShaderStage::eVertex;
    shader_descs[ 0 ].bytecode_size         = vert_code.size() * sizeof(vert_code[ 0 ]);
    shader_descs[ 0 ].bytecode              = vert_code.data();
    shader_descs[ 1 ].stage                 = ShaderStage::eFragment;
    shader_descs[ 1 ].bytecode_size         = phong_frag_code.size() * sizeof(phong_frag_code[ 0 ]);
    shader_descs[ 1 ].bytecode              = phong_frag_code.data();
    shader_descs[ 2 ].stage                 = ShaderStage::eFragment;
    shader_descs[ 2 ].bytecode_size         = color_frag_code.size() * sizeof(color_frag_code[ 0 ]);
    shader_descs[ 2 ].bytecode              = color_frag_code.data();

    Shader* shaders[ shader_count ] = {};
    for (u32 i = 0; i < shader_count; ++i)
    {
        create_shader(m_context->device(), &shader_descs[ i ], &shaders[ i ]);
    }

    create_descriptor_set_layout(m_context->device(), 2, shaders, &m_data.descriptor_set_layout);

    PipelineDesc pipeline_desc{};
    setup_vertex_layout(pipeline_desc.vertex_layout);
    pipeline_desc.rasterizer_desc.cull_mode    = CullMode::eBack;
    pipeline_desc.rasterizer_desc.front_face   = FrontFace::eCounterClockwise;
    pipeline_desc.depth_state_desc.depth_test  = true;
    pipeline_desc.depth_state_desc.depth_write = true;
    pipeline_desc.depth_state_desc.compare_op  = CompareOp::eLess;
    pipeline_desc.shader_count                 = 2;
    pipeline_desc.shaders[ 0 ]                 = shaders[ 0 ];
    pipeline_desc.shaders[ 1 ]                 = shaders[ 1 ];
    pipeline_desc.descriptor_set_layout        = m_data.descriptor_set_layout;
    pipeline_desc.render_pass                  = m_data.render_pass;

    create_graphics_pipeline(m_context->device(), &pipeline_desc, &m_data.phong_pipeline);
    pipeline_desc.shaders[ 1 ] = shaders[ 2 ];
    create_graphics_pipeline(m_context->device(), &pipeline_desc, &m_data.color_pipeline);

    for (u32 i = 0; i < shader_count; ++i)
    {
        destroy_shader(m_context->device(), shaders[ i ]);
    }

    BufferLoadDesc ubo_load_desc{};
    ubo_load_desc.size          = sizeof(CameraData);
    BufferDesc& buffer_desc     = ubo_load_desc.buffer_desc;
    buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;
    buffer_desc.size            = ubo_load_desc.size;

    ResourceManager::load_buffer(m_data.uniform_buffer, &ubo_load_desc);

    create_default_resources();

    DescriptorSetDesc set_desc{};
    set_desc.index                 = 0;
    set_desc.descriptor_set_layout = m_data.descriptor_set_layout;
    create_descriptor_set(m_context->device(), &set_desc, &m_data.per_frame_set);
    set_desc.index = 1;
    create_descriptor_set(m_context->device(), &set_desc, &m_data.per_material_set);

    DescriptorBufferDesc descriptor_buffer_desc = {};
    descriptor_buffer_desc.offset               = 0;
    descriptor_buffer_desc.range                = sizeof(CameraData);
    descriptor_buffer_desc.buffer               = m_data.uniform_buffer->get();

    DescriptorImageDesc descriptor_sampler_desc = {};
    descriptor_sampler_desc.sampler             = m_resources.default_sampler;

    DescriptorWriteDesc per_frame_set_writes[ 2 ]     = {};
    per_frame_set_writes[ 0 ].binding                 = 0;
    per_frame_set_writes[ 0 ].descriptor_type         = DescriptorType::eUniformBuffer;
    per_frame_set_writes[ 0 ].descriptor_count        = 1;
    per_frame_set_writes[ 0 ].descriptor_buffer_descs = &descriptor_buffer_desc;
    per_frame_set_writes[ 1 ].binding                 = 1;
    per_frame_set_writes[ 1 ].descriptor_count        = 1;
    per_frame_set_writes[ 1 ].descriptor_type         = DescriptorType::eSampler;
    per_frame_set_writes[ 1 ].descriptor_image_descs  = &descriptor_sampler_desc;

    update_descriptor_set(m_context->device(), m_data.per_frame_set, 2, per_frame_set_writes);

    DescriptorImageDesc descriptor_image_desc = {};
    descriptor_image_desc.image               = m_resources.default_albedo->get();
    descriptor_image_desc.resource_state      = ResourceState::eShaderReadOnly;

    DescriptorWriteDesc per_material_set_write    = {};
    per_material_set_write.binding                = 0;
    per_material_set_write.descriptor_count       = 1;
    per_material_set_write.descriptor_type        = DescriptorType::eSampledImage;
    per_material_set_write.descriptor_image_descs = &descriptor_image_desc;

    update_descriptor_set(m_context->device(), m_data.per_material_set, 1, &per_material_set_write);
}

void Renderer3D::shutdown()
{
    device_wait_idle(m_context->device());

    for (u32 i = 0; i < GraphicContext::frame_count(); ++i)
    {
        for (DescriptorSet* set : m_frame_data.free_set_lists[ i ])
        {
            destroy_descriptor_set(m_context->device(), set);
        }
    }

    destroy_descriptor_set(m_context->device(), m_data.per_material_set);
    destroy_descriptor_set(m_context->device(), m_data.per_frame_set);
    destroy_default_resources();
    ResourceManager::release_buffer(m_data.uniform_buffer);
    destroy_descriptor_set_layout(m_context->device(), m_data.descriptor_set_layout);
    destroy_pipeline(m_context->device(), m_data.phong_pipeline);
    destroy_pipeline(m_context->device(), m_data.color_pipeline);
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
    // Just because frame always ends with shader read only
    image_barriers[ 0 ].new_state = ResourceState::eShaderReadOnly;
    image_barriers[ 1 ].image     = m_data.output_depth_image;
    image_barriers[ 1 ].src_queue = m_context->queue();
    image_barriers[ 1 ].dst_queue = m_context->queue();
    image_barriers[ 1 ].old_state = ResourceState::eUndefined;
    image_barriers[ 1 ].new_state = ResourceState::eDepthStencilWrite;

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

void Renderer3D::begin_frame(const Camera& camera)
{
    for (DescriptorSet* set : m_frame_data.free_set_lists[ m_context->frame_index() ])
    {
        destroy_descriptor_set(m_context->device(), set);
    }
    m_frame_data.free_set_lists[ m_context->frame_index() ].clear();

    map_memory(m_context->device(), m_data.uniform_buffer->get());
    std::memcpy(m_data.uniform_buffer->get()->mapped_memory, &camera.get_data(), sizeof(CameraData));
    unmap_memory(m_context->device(), m_data.uniform_buffer->get());

    m_push_constant_block.viewer_position = Vector4(camera.get_position(), 0.0);

    ImageBarrier to_color_attachment{};
    to_color_attachment.image     = m_data.output_image;
    to_color_attachment.old_state = ResourceState::eShaderReadOnly;
    to_color_attachment.new_state = ResourceState::eColorAttachment;
    to_color_attachment.src_queue = m_context->queue();
    to_color_attachment.dst_queue = m_context->queue();

    auto* cmd = m_context->acquire_cmd();
    cmd_barrier(cmd, 0, nullptr, 1, &to_color_attachment);

    RenderPassBeginDesc rp_begin_desc{};
    rp_begin_desc.clear_values[ 0 ].color[ 0 ] = 0.52;
    rp_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.8;
    rp_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.92;
    rp_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0;
    rp_begin_desc.clear_values[ 1 ].depth      = 1.0f;
    rp_begin_desc.clear_values[ 1 ].stencil    = 0;
    rp_begin_desc.render_pass                  = m_data.render_pass;

    cmd_begin_render_pass(cmd, &rp_begin_desc);

    cmd_set_viewport(cmd, 0, 0, m_data.viewport_width, m_data.viewport_height, 0.0f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, m_data.viewport_width, m_data.viewport_height);
    cmd_bind_pipeline(cmd, m_data.phong_pipeline);
    cmd_bind_descriptor_set(cmd, 0, m_data.per_frame_set, m_data.phong_pipeline);
}

void Renderer3D::set_light_model(LightModel light_model)
{
    auto* cmd = m_context->acquire_cmd();
    switch (light_model)
    {
    case LightModel::eNone:
        cmd_bind_pipeline(cmd, m_data.color_pipeline);
        break;
    case LightModel::ePhong:
        cmd_bind_pipeline(cmd, m_data.color_pipeline);
        break;
    default:
        break;
    }
}

void Renderer3D::draw_geometry(const Matrix4& transform, const Ref<Geometry> geometry)
{
    auto* cmd = m_context->acquire_cmd();

    cmd_bind_descriptor_set(cmd, 1, m_data.per_material_set, m_data.phong_pipeline);
    for (auto& node : geometry->nodes())
    {
        m_push_constant_block.model = transform * node.transform;
        cmd_push_constants(cmd, m_data.phong_pipeline, 0, sizeof(PushConstantBlock), &m_push_constant_block);
        cmd_bind_vertex_buffer(cmd, node.vertex_buffer->get());
        cmd_bind_index_buffer_u32(cmd, node.index_buffer->get());
        cmd_draw_indexed(cmd, node.index_count, 1, 0, 0, 0);
    }
}

void Renderer3D::draw_geometry(const Matrix4& transform, const Ref<Geometry> geometry, const Ref<Image> image)
{
    auto* cmd = m_context->acquire_cmd();

    // TODO: its very bad solution
    DescriptorSet*    set = nullptr;
    DescriptorSetDesc set_desc{};
    set_desc.index                 = 1;
    set_desc.descriptor_set_layout = m_data.descriptor_set_layout;

    create_descriptor_set(m_context->device(), &set_desc, &set);

    DescriptorImageDesc descriptor_image_desc = {};
    descriptor_image_desc.image               = image->get();
    descriptor_image_desc.resource_state      = ResourceState::eShaderReadOnly;

    DescriptorWriteDesc set_write    = {};
    set_write.binding                = 0;
    set_write.descriptor_count       = 1;
    set_write.descriptor_type        = DescriptorType::eSampledImage;
    set_write.descriptor_image_descs = &descriptor_image_desc;

    update_descriptor_set(m_context->device(), set, 1, &set_write);

    m_frame_data.free_set_lists[ m_context->frame_index() ].push_back(set);
    cmd_bind_descriptor_set(cmd, 1, set, m_data.phong_pipeline);

    for (auto& node : geometry->nodes())
    {
        m_push_constant_block.model = transform * node.transform;
        cmd_push_constants(cmd, m_data.phong_pipeline, 0, sizeof(PushConstantBlock), &m_push_constant_block);
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
