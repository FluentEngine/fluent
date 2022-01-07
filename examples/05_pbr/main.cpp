#include <iostream>

#include "fluent/fluent.hpp"

using namespace fluent;

struct CameraUBO
{
    Matrix4 projection;
    Matrix4 view;
};

struct PushConstantBlock
{
    Matrix4 model;
    Vector4 view_position = Vector4(0.0, 0.0, 2.0, 0.0);
};

struct TextureIndices
{
    i32 albedo = -1;
    i32 normal = -1;
    i32 metallic_roughness = -1;
    i32 ao = -1;
    i32 emissive = -1;
};

static constexpr u32 FRAME_COUNT = 2;
u32 frame_index = 0;

Renderer renderer;
Device device;
Queue queue;
CommandPool command_pool;
Semaphore image_available_semaphores[ FRAME_COUNT ];
Semaphore rendering_finished_semaphores[ FRAME_COUNT ];
Fence in_flight_fences[ FRAME_COUNT ];
bool command_buffers_recorded[ FRAME_COUNT ];

Swapchain swapchain;
CommandBuffer command_buffers[ FRAME_COUNT ];

// skybox
DescriptorSetLayout skybox_descriptor_set_layout;
DescriptorSet skybox_descriptor_set;
Sampler skybox_sampler;
// Image pano_skybox;
Pipeline skybox_pipeline;
Buffer ubo_buffer;

// IBL
Image environment_map;
Image brdf_integration_map;
Image irradiance_map;
Image specular_map;

// model
TextureIndices texture_indices;
Model model;
DescriptorSetLayout pbr_descriptor_set_layout;
DescriptorSet pbr_descriptor_set;
Pipeline pbr_pipeline;
Sampler pbr_sampler;

CameraUBO ubo;

void compute_pbr_maps()
{
    static const u32 skybox_size = 1024;
    // TODO: mip levels
    static const u32 skybox_mips = /* ( u32 ) log2(skybox_size) */ +1;
    static const u32 irradiance_size = 128;
    static const u32 specular_size = 128;
    static const u32 brdf_integration_size = 512;
    static const u32 specular_mips = /* ( u32 ) log2(specular_size) + */ 1;

    SamplerDesc sampler_desc{};
    sampler_desc.min_filter = Filter::eLinear;
    sampler_desc.mag_filter = Filter::eLinear;
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.address_mode_u = SamplerAddressMode::eRepeat;
    sampler_desc.address_mode_v = SamplerAddressMode::eRepeat;
    sampler_desc.address_mode_w = SamplerAddressMode::eRepeat;
    sampler_desc.min_lod = 0;
    sampler_desc.max_lod = 16;

    ImageDesc skybox_image_desc{};
    skybox_image_desc.layer_count = 6;
    skybox_image_desc.depth = 1;
    skybox_image_desc.format = Format::eR32G32B32A32Sfloat;
    skybox_image_desc.height = skybox_size;
    skybox_image_desc.width = skybox_size;
    skybox_image_desc.mip_levels = skybox_mips;
    skybox_image_desc.sample_count = SampleCount::e1;
    skybox_image_desc.resource_state = ResourceState::eStorage;
    skybox_image_desc.descriptor_type = DescriptorType::eStorageImage | DescriptorType::eSampledImage;

    ImageDesc irr_image_desc{};
    irr_image_desc.layer_count = 6;
    irr_image_desc.depth = 1;
    irr_image_desc.format = Format::eR32G32B32A32Sfloat;
    irr_image_desc.width = irradiance_size;
    irr_image_desc.height = irradiance_size;
    irr_image_desc.mip_levels = 1;
    irr_image_desc.sample_count = SampleCount::e1;
    irr_image_desc.resource_state = ResourceState::eStorage;
    irr_image_desc.descriptor_type = DescriptorType::eSampledImage | DescriptorType::eStorageImage;

    ImageDesc specular_image_desc{};
    specular_image_desc.layer_count = 6;
    specular_image_desc.depth = 1;
    specular_image_desc.format = Format::eR32G32B32A32Sfloat;
    specular_image_desc.width = specular_size;
    specular_image_desc.height = specular_size;
    specular_image_desc.mip_levels = specular_mips; // TODO
    specular_image_desc.sample_count = SampleCount::e1;
    specular_image_desc.resource_state = ResourceState::eStorage;
    specular_image_desc.descriptor_type = DescriptorType::eSampledImage | DescriptorType::eStorageImage;

    // Create empty texture for BRDF integration map.
    ImageDesc brdf_integration_image_desc{};
    brdf_integration_image_desc.width = brdf_integration_size;
    brdf_integration_image_desc.height = brdf_integration_size;
    brdf_integration_image_desc.depth = 1;
    brdf_integration_image_desc.layer_count = 1;
    brdf_integration_image_desc.mip_levels = 1;
    brdf_integration_image_desc.format = Format::eR32G32Sfloat;
    brdf_integration_image_desc.sample_count = SampleCount::e1;
    brdf_integration_image_desc.resource_state = ResourceState::eStorage;
    brdf_integration_image_desc.descriptor_type = DescriptorType::eSampledImage | DescriptorType::eStorageImage;

    Sampler skybox_sampler = create_sampler(device, sampler_desc);
    Image pano_skybox = load_image_from_dds_file(device, "LA_Helipad.dds", ResourceState::eShaderReadOnly, false);
    Shader pano_to_cube_shader = create_shader(device, "pano_to_cube.comp.glsl.spv", ShaderStage::eCompute);

    // precomputed skybox
    environment_map = create_image(device, skybox_image_desc);
    irradiance_map = create_image(device, irr_image_desc);
    specular_map = create_image(device, specular_image_desc);
    brdf_integration_map = create_image(device, brdf_integration_image_desc);

    DescriptorSetLayout pano_to_cube_dsl = create_descriptor_set_layout(device, 1, &pano_to_cube_shader);

    PipelineDesc pipeline_desc{};
    pipeline_desc.descriptor_set_layout = &pano_to_cube_dsl;

    Pipeline pano_to_cube_pipeline = create_compute_pipeline(device, pipeline_desc);

    DescriptorSetDesc ds_desc{};
    ds_desc.descriptor_set_layout = &pano_to_cube_dsl;

    DescriptorSet ds_pano_to_cube = create_descriptor_set(device, ds_desc);

    DescriptorImageDesc descriptor_sampler_desc{};
    descriptor_sampler_desc.sampler = &skybox_sampler;

    DescriptorImageDesc descriptor_image_desc{};
    descriptor_image_desc.image = &pano_skybox;
    descriptor_image_desc.resource_state = ResourceState::eShaderReadOnly;

    DescriptorImageDesc descriptor_output_image_desc{};
    descriptor_output_image_desc.image = &environment_map;
    descriptor_output_image_desc.resource_state = ResourceState::eStorage;

    DescriptorWriteDesc descriptor_write_descs[ 3 ] = {};
    descriptor_write_descs[ 0 ].binding = 0;
    descriptor_write_descs[ 0 ].descriptor_count = 1;
    descriptor_write_descs[ 0 ].descriptor_type = DescriptorType::eSampledImage;
    descriptor_write_descs[ 0 ].descriptor_image_descs = &descriptor_image_desc;
    descriptor_write_descs[ 1 ].binding = 1;
    descriptor_write_descs[ 1 ].descriptor_count = 1;
    descriptor_write_descs[ 1 ].descriptor_type = DescriptorType::eSampler;
    descriptor_write_descs[ 1 ].descriptor_image_descs = &descriptor_sampler_desc;
    descriptor_write_descs[ 2 ].binding = 2;
    descriptor_write_descs[ 2 ].descriptor_count = 1;
    descriptor_write_descs[ 2 ].descriptor_type = DescriptorType::eStorageImage;
    descriptor_write_descs[ 2 ].descriptor_image_descs = &descriptor_output_image_desc;
    update_descriptor_set(device, ds_pano_to_cube, 3, descriptor_write_descs);

    Shader brdf_integration_shader = create_shader(device, "brdf_integration.comp.glsl.spv", ShaderStage::eCompute);

    DescriptorSetLayout brdf_integration_dsl = create_descriptor_set_layout(device, 1, &brdf_integration_shader);

    DescriptorSetDesc brdf_integration_set_desc{};
    brdf_integration_set_desc.descriptor_set_layout = &brdf_integration_dsl;

    DescriptorSet brdf_integration_set = create_descriptor_set(device, brdf_integration_set_desc);

    DescriptorImageDesc brdf_integration_image_set_desc{};
    brdf_integration_image_set_desc.resource_state = ResourceState::eStorage;
    brdf_integration_image_set_desc.image = &brdf_integration_map;

    DescriptorWriteDesc descriptor_write_desc{};
    descriptor_write_desc.binding = 0;
    descriptor_write_desc.descriptor_count = 1;
    descriptor_write_desc.descriptor_type = DescriptorType::eStorageImage;
    descriptor_write_desc.descriptor_image_descs = &brdf_integration_image_set_desc;

    update_descriptor_set(device, brdf_integration_set, 1, &descriptor_write_desc);

    PipelineDesc brdf_integration_pipeline_desc{};
    brdf_integration_pipeline_desc.descriptor_set_layout = &brdf_integration_dsl;

    Pipeline brdf_integration_pipeline = create_compute_pipeline(device, brdf_integration_pipeline_desc);

    Shader irradiance_shader = create_shader(device, "irradiance.comp.glsl.spv", ShaderStage::eCompute);
    DescriptorSetLayout irradiance_dsl = create_descriptor_set_layout(device, 1, &irradiance_shader);
    DescriptorSetDesc irradiance_set_desc{};
    irradiance_set_desc.descriptor_set_layout = &irradiance_dsl;
    DescriptorSet irradiance_set = create_descriptor_set(device, irradiance_set_desc);

    DescriptorImageDesc irradiance_set_images[ 2 ] = {};
    irradiance_set_images[ 0 ].image = &environment_map;
    irradiance_set_images[ 0 ].sampler = &skybox_sampler;
    irradiance_set_images[ 0 ].resource_state = ResourceState::eStorage;
    irradiance_set_images[ 1 ].image = &irradiance_map;
    irradiance_set_images[ 1 ].resource_state = ResourceState::eStorage;

    descriptor_write_descs[ 0 ].binding = 0;
    descriptor_write_descs[ 0 ].descriptor_count = 1;
    descriptor_write_descs[ 0 ].descriptor_type = DescriptorType::eCombinedImageSampler;
    descriptor_write_descs[ 0 ].descriptor_image_descs = &irradiance_set_images[ 0 ];
    descriptor_write_descs[ 1 ].binding = 1;
    descriptor_write_descs[ 1 ].descriptor_count = 1;
    descriptor_write_descs[ 1 ].descriptor_type = DescriptorType::eStorageImage;
    descriptor_write_descs[ 1 ].descriptor_image_descs = &irradiance_set_images[ 1 ];

    update_descriptor_set(device, irradiance_set, 2, descriptor_write_descs);

    PipelineDesc irradiance_pipeline_desc{};
    irradiance_pipeline_desc.descriptor_set_layout = &irradiance_dsl;

    Pipeline irradiance_pipeline = create_compute_pipeline(device, irradiance_pipeline_desc);

    Shader specular_shader = create_shader(device, "specular.comp.glsl.spv", ShaderStage::eCompute);

    DescriptorSetLayout specular_set_layout = create_descriptor_set_layout(device, 1, &specular_shader);
    DescriptorSetDesc specular_set_desc{};
    specular_set_desc.descriptor_set_layout = &specular_set_layout;
    DescriptorSet specular_set = create_descriptor_set(device, specular_set_desc);

    DescriptorImageDesc specular_set_images[ 2 ] = {};
    specular_set_images[ 0 ].image = &environment_map;
    specular_set_images[ 0 ].sampler = &skybox_sampler;
    specular_set_images[ 0 ].resource_state = ResourceState::eStorage;
    specular_set_images[ 1 ].image = &specular_map;
    specular_set_images[ 1 ].resource_state = ResourceState::eStorage;

    descriptor_write_descs[ 0 ].binding = 0;
    descriptor_write_descs[ 0 ].descriptor_count = 1;
    descriptor_write_descs[ 0 ].descriptor_type = DescriptorType::eCombinedImageSampler;
    descriptor_write_descs[ 0 ].descriptor_image_descs = &specular_set_images[ 0 ];
    descriptor_write_descs[ 1 ].binding = 1;
    descriptor_write_descs[ 1 ].descriptor_count = 1;
    descriptor_write_descs[ 1 ].descriptor_type = DescriptorType::eStorageImage;
    descriptor_write_descs[ 1 ].descriptor_image_descs = &specular_set_images[ 1 ];

    update_descriptor_set(device, specular_set, 2, descriptor_write_descs);

    PipelineDesc specular_pipeline_desc{};
    specular_pipeline_desc.descriptor_set_layout = &specular_set_layout;

    Pipeline specular_pipeline = create_compute_pipeline(device, specular_pipeline_desc);

    auto& cmd = command_buffers[ 0 ];

    begin_command_buffer(cmd);
    // BRDF
    cmd_bind_pipeline(cmd, brdf_integration_pipeline);
    cmd_bind_descriptor_set(cmd, brdf_integration_set, brdf_integration_pipeline);
    cmd_dispatch(cmd, brdf_integration_size / 16, brdf_integration_size / 16, 1);

    // Environment map
    cmd_bind_pipeline(cmd, pano_to_cube_pipeline);
    cmd_bind_descriptor_set(cmd, ds_pano_to_cube, pano_to_cube_pipeline);
    struct
    {
        uint32_t mip;
        uint32_t textureSize;
    } root_constant_data = { 0, skybox_size };

    for (uint32_t i = 0; i < skybox_mips; ++i)
    {
        root_constant_data.mip = i;
        cmd_push_constants(cmd, pano_to_cube_pipeline, 0, sizeof(root_constant_data), &root_constant_data);
        cmd_dispatch(
            cmd, std::max(1u, ( u32 ) (root_constant_data.textureSize >> i) / 16),
            std::max(1u, ( u32 ) (root_constant_data.textureSize >> i) / 16), 6);
    }

    // Irradiance map
    cmd_bind_pipeline(cmd, irradiance_pipeline);
    cmd_bind_descriptor_set(cmd, irradiance_set, irradiance_pipeline);
    cmd_dispatch(cmd, irradiance_size / 16, irradiance_size / 16, 6);

    // Specular map
    struct PrecomputeSkySpecularData
    {
        u32 mipSize;
        f32 roughness;
    } specular_data = { specular_size >> 0, 0 };

    cmd_bind_pipeline(cmd, specular_pipeline);
    cmd_bind_descriptor_set(cmd, specular_set, specular_pipeline);
    cmd_push_constants(cmd, specular_pipeline, 0, sizeof(specular_data), &specular_data);
    cmd_dispatch(cmd, specular_size / 16, specular_size / 16, 6);

    ImageBarrier to_sampled_barrier{};
    to_sampled_barrier.image = &environment_map;
    to_sampled_barrier.old_state = ResourceState::eStorage;
    to_sampled_barrier.new_state = ResourceState::eShaderReadOnly;
    to_sampled_barrier.src_queue = &queue;
    to_sampled_barrier.dst_queue = &queue;
    cmd_barrier(cmd, 0, nullptr, 1, &to_sampled_barrier);
    to_sampled_barrier.image = &brdf_integration_map;
    cmd_barrier(cmd, 0, nullptr, 1, &to_sampled_barrier);
    to_sampled_barrier.image = &irradiance_map;
    cmd_barrier(cmd, 0, nullptr, 1, &to_sampled_barrier);
    to_sampled_barrier.image = &specular_map;
    cmd_barrier(cmd, 0, nullptr, 1, &to_sampled_barrier);

    end_command_buffer(cmd);

    immediate_submit(queue, cmd);

    destroy_descriptor_set(device, specular_set);
    destroy_descriptor_set_layout(device, specular_set_layout);
    destroy_pipeline(device, specular_pipeline);
    destroy_shader(device, specular_shader);
    destroy_descriptor_set(device, irradiance_set);
    destroy_descriptor_set_layout(device, irradiance_dsl);
    destroy_pipeline(device, irradiance_pipeline);
    destroy_shader(device, irradiance_shader);
    destroy_descriptor_set(device, brdf_integration_set);
    destroy_descriptor_set_layout(device, brdf_integration_dsl);
    destroy_pipeline(device, brdf_integration_pipeline);
    destroy_shader(device, brdf_integration_shader);
    destroy_descriptor_set(device, ds_pano_to_cube);
    destroy_descriptor_set_layout(device, pano_to_cube_dsl);
    destroy_pipeline(device, pano_to_cube_pipeline);
    destroy_shader(device, pano_to_cube_shader);
    destroy_image(device, pano_skybox);
    destroy_sampler(device, skybox_sampler);
}
void destroy_pbr_maps()
{
    destroy_image(device, environment_map);
    destroy_image(device, brdf_integration_map);
    destroy_image(device, irradiance_map);
    destroy_image(device, specular_map);
}

void begin_frame(u32& image_index)
{
    if (!command_buffers_recorded[ frame_index ])
    {
        wait_for_fences(device, 1, &in_flight_fences[ frame_index ]);
        reset_fences(device, 1, &in_flight_fences[ frame_index ]);
        command_buffers_recorded[ frame_index ] = true;
    }

    acquire_next_image(device, swapchain, image_available_semaphores[ frame_index ], {}, image_index);
}
void end_frame(u32 image_index)
{
    auto& cmd = command_buffers[ frame_index ];

    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.wait_semaphore_count = 1;
    queue_submit_desc.wait_semaphores = &image_available_semaphores[ frame_index ];
    queue_submit_desc.command_buffer_count = 1;
    queue_submit_desc.command_buffers = &cmd;
    queue_submit_desc.signal_semaphore_count = 1;
    queue_submit_desc.signal_semaphores = &rendering_finished_semaphores[ frame_index ];
    queue_submit_desc.signal_fence = &in_flight_fences[ frame_index ];

    queue_submit(queue, queue_submit_desc);

    QueuePresentDesc queue_present_desc{};
    queue_present_desc.wait_semaphore_count = 1;
    queue_present_desc.wait_semaphores = &rendering_finished_semaphores[ frame_index ];
    queue_present_desc.swapchain = &swapchain;
    queue_present_desc.image_index = image_index;

    queue_present(queue, queue_present_desc);

    command_buffers_recorded[ frame_index ] = false;
    frame_index = (frame_index + 1) % FRAME_COUNT;
}

void load_skybox()
{
    SamplerDesc sampler_desc{};
    sampler_desc.min_filter = Filter::eLinear;
    sampler_desc.mag_filter = Filter::eLinear;
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.address_mode_u = SamplerAddressMode::eRepeat;
    sampler_desc.address_mode_v = SamplerAddressMode::eRepeat;
    sampler_desc.address_mode_w = SamplerAddressMode::eRepeat;
    sampler_desc.min_lod = 0;
    sampler_desc.max_lod = 16;

    skybox_sampler = create_sampler(device, sampler_desc);

    Shader shaders[ 2 ] = {};
    shaders[ 0 ] = create_shader(device, "skybox.vert.glsl.spv", ShaderStage::eVertex);
    shaders[ 1 ] = create_shader(device, "skybox.frag.glsl.spv", ShaderStage::eFragment);

    skybox_descriptor_set_layout = create_descriptor_set_layout(device, 2, shaders);

    PipelineDesc pipeline_desc{};
    pipeline_desc.binding_desc_count = 0;
    pipeline_desc.attribute_desc_count = 0;
    pipeline_desc.depth_state_desc.depth_test = false;
    pipeline_desc.depth_state_desc.depth_test = false;
    pipeline_desc.depth_state_desc.compare_op = CompareOp::eLessOrEqual;
    pipeline_desc.rasterizer_desc.cull_mode = CullMode::eNone;
    pipeline_desc.rasterizer_desc.front_face = FrontFace::eCounterClockwise;
    pipeline_desc.render_pass = &swapchain.render_passes[ 0 ];
    pipeline_desc.descriptor_set_layout = &skybox_descriptor_set_layout;

    skybox_pipeline = create_graphics_pipeline(device, pipeline_desc);

    DescriptorSetDesc descriptor_set_desc{};
    descriptor_set_desc.descriptor_set_layout = &skybox_descriptor_set_layout;

    skybox_descriptor_set = create_descriptor_set(device, descriptor_set_desc);

    DescriptorBufferDesc descriptor_buffer_desc{};
    descriptor_buffer_desc.offset = 0;
    descriptor_buffer_desc.range = sizeof(CameraUBO);
    descriptor_buffer_desc.buffer = &ubo_buffer;

    DescriptorImageDesc descriptor_image_desc{};
    descriptor_image_desc.image = &environment_map;
    descriptor_image_desc.sampler = &skybox_sampler;
    descriptor_image_desc.resource_state = ResourceState::eShaderReadOnly;

    DescriptorWriteDesc descriptor_write_descs[ 2 ] = {};
    descriptor_write_descs[ 0 ].binding = 0;
    descriptor_write_descs[ 0 ].descriptor_count = 1;
    descriptor_write_descs[ 0 ].descriptor_buffer_descs = &descriptor_buffer_desc;
    descriptor_write_descs[ 0 ].descriptor_type = DescriptorType::eUniformBuffer;
    descriptor_write_descs[ 1 ].binding = 1;
    descriptor_write_descs[ 1 ].descriptor_count = 1;
    descriptor_write_descs[ 1 ].descriptor_image_descs = &descriptor_image_desc;
    descriptor_write_descs[ 1 ].descriptor_type = DescriptorType::eCombinedImageSampler;

    update_descriptor_set(device, skybox_descriptor_set, 2, descriptor_write_descs);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(device, shaders[ i ]);
    }
}

void draw_skybox(const CommandBuffer& cmd)
{
    cmd_bind_pipeline(cmd, skybox_pipeline);
    cmd_bind_descriptor_set(cmd, skybox_descriptor_set, skybox_pipeline);
    cmd_draw(cmd, 36, 1, 0, 0);
}

void release_skybox()
{
    destroy_pipeline(device, skybox_pipeline);
    destroy_sampler(device, skybox_sampler);
    destroy_descriptor_set(device, skybox_descriptor_set);
    destroy_descriptor_set_layout(device, skybox_descriptor_set_layout);
}

void load_model()
{
    LoadModelDescription load_model_desc{};
    load_model_desc.filename = "damaged-helmet/DamagedHelmet.gltf";
    load_model_desc.load_bitangents = true;
    load_model_desc.load_normals = true;
    load_model_desc.load_tangents = true;
    load_model_desc.load_tex_coords = true;
    load_model_desc.flip_uvs = true;

    ModelLoader model_loader;
    model = model_loader.load(&device, load_model_desc);

    SamplerDesc sampler_desc{};
    sampler_desc.mipmap_mode = SamplerMipmapMode::eLinear;
    sampler_desc.min_lod = 0;
    sampler_desc.max_lod = 1000;

    pbr_sampler = create_sampler(device, sampler_desc);

    Shader shaders[ 2 ] = {};
    shaders[ 0 ] = create_shader(device, "pbr.vert.glsl.spv", ShaderStage::eVertex);
    shaders[ 1 ] = create_shader(device, "pbr.frag.glsl.spv", ShaderStage::eFragment);

    pbr_descriptor_set_layout = create_descriptor_set_layout(device, 2, shaders);

    PipelineDesc pipeline_desc{};
    model_loader.get_vertex_binding_description(pipeline_desc.binding_desc_count, pipeline_desc.binding_descs);
    model_loader.get_vertex_attribute_description(pipeline_desc.attribute_desc_count, pipeline_desc.attribute_descs);
    pipeline_desc.depth_state_desc.depth_test = true;
    pipeline_desc.depth_state_desc.depth_write = true;
    pipeline_desc.depth_state_desc.compare_op = CompareOp::eLess;
    pipeline_desc.rasterizer_desc.cull_mode = CullMode::eBack;
    pipeline_desc.rasterizer_desc.front_face = FrontFace::eCounterClockwise;
    pipeline_desc.render_pass = &swapchain.render_passes[ 0 ];
    pipeline_desc.descriptor_set_layout = &pbr_descriptor_set_layout;

    pbr_pipeline = create_graphics_pipeline(device, pipeline_desc);

    DescriptorSetDesc descriptor_set_desc{};
    descriptor_set_desc.descriptor_set_layout = &pbr_descriptor_set_layout;

    pbr_descriptor_set = create_descriptor_set(device, descriptor_set_desc);

    std::vector<DescriptorImageDesc> descriptor_image_descs(model.textures.size() + 3);
    for (uint32_t i = 0; i < descriptor_image_descs.size() - 3; ++i)
    {
        descriptor_image_descs[ i ] = {};
        descriptor_image_descs[ i ].image = &model.textures[ i ];
        descriptor_image_descs[ i ].resource_state = ResourceState::eShaderReadOnly;
    }

    u32 descriptors_count = model.textures.size();

    DescriptorBufferDesc descriptor_buffer_desc{};
    descriptor_buffer_desc.offset = 0;
    descriptor_buffer_desc.range = sizeof(CameraUBO);
    descriptor_buffer_desc.buffer = &ubo_buffer;

    DescriptorImageDesc descriptor_sampler_desc{};
    descriptor_sampler_desc.sampler = &pbr_sampler;

    DescriptorWriteDesc descriptor_write_descs[ 6 ] = {};
    descriptor_write_descs[ 0 ].binding = 0;
    descriptor_write_descs[ 0 ].descriptor_count = 1;
    descriptor_write_descs[ 0 ].descriptor_buffer_descs = &descriptor_buffer_desc;
    descriptor_write_descs[ 0 ].descriptor_type = DescriptorType::eUniformBuffer;
    descriptor_write_descs[ 1 ].binding = 1;
    descriptor_write_descs[ 1 ].descriptor_count = 1;
    descriptor_write_descs[ 1 ].descriptor_image_descs = &descriptor_sampler_desc;
    descriptor_write_descs[ 1 ].descriptor_type = DescriptorType::eSampler;
    descriptor_write_descs[ 2 ].binding = 2;
    descriptor_write_descs[ 2 ].descriptor_count = descriptor_image_descs.size();
    descriptor_write_descs[ 2 ].descriptor_image_descs = descriptor_image_descs.data();
    descriptor_write_descs[ 2 ].descriptor_type = DescriptorType::eSampledImage;
    auto& irradiance_map_desc = descriptor_image_descs[ descriptors_count++ ];
    irradiance_map_desc = {};
    irradiance_map_desc.image = &irradiance_map;
    irradiance_map_desc.sampler = &skybox_sampler; // TODO:
    irradiance_map_desc.resource_state = ResourceState::eShaderReadOnly;
    descriptor_write_descs[ 3 ].binding = 3;
    descriptor_write_descs[ 3 ].descriptor_count = 1;
    descriptor_write_descs[ 3 ].descriptor_image_descs = &irradiance_map_desc;
    descriptor_write_descs[ 3 ].descriptor_type = DescriptorType::eCombinedImageSampler;
    auto& specular_map_desc = descriptor_image_descs[ descriptors_count++ ];
    specular_map_desc = {};
    specular_map_desc.image = &specular_map;
    specular_map_desc.sampler = &skybox_sampler; // TODO:
    specular_map_desc.resource_state = ResourceState::eShaderReadOnly;
    descriptor_write_descs[ 4 ].binding = 4;
    descriptor_write_descs[ 4 ].descriptor_count = 1;
    descriptor_write_descs[ 4 ].descriptor_image_descs = &specular_map_desc;
    descriptor_write_descs[ 4 ].descriptor_type = DescriptorType::eCombinedImageSampler;
    auto& brdf_map_desc = descriptor_image_descs[ descriptors_count++ ];
    brdf_map_desc = {};
    brdf_map_desc.image = &brdf_integration_map;
    brdf_map_desc.resource_state = ResourceState::eShaderReadOnly;
    descriptor_write_descs[ 5 ].binding = 5;
    descriptor_write_descs[ 5 ].descriptor_count = 1;
    descriptor_write_descs[ 5 ].descriptor_image_descs = &brdf_map_desc;
    descriptor_write_descs[ 5 ].descriptor_type = DescriptorType::eSampledImage;
    update_descriptor_set(device, pbr_descriptor_set, 6, descriptor_write_descs);

    for (u32 i = 0; i < 2; ++i)
    {
        destroy_shader(device, shaders[ i ]);
    }
}

void release_model()
{
    destroy_model(device, model);
    destroy_pipeline(device, pbr_pipeline);
    destroy_sampler(device, pbr_sampler);
    destroy_descriptor_set(device, pbr_descriptor_set);
    destroy_descriptor_set_layout(device, pbr_descriptor_set_layout);
}

void draw_model(const CommandBuffer& cmd)
{
    cmd_bind_pipeline(cmd, pbr_pipeline);
    cmd_bind_descriptor_set(cmd, pbr_descriptor_set, pbr_pipeline);

    PushConstantBlock pcb{};
    pcb.model = Matrix4(1.0);
    pcb.model = translate(pcb.model, Vector3(0.0, 0.0, 0.0));
    pcb.model = glm::scale(pcb.model, Vector3(0.3));
    pcb.model = rotate(pcb.model, radians(get_time() / 10), Vector3(0.0, 1.0, 0.0));

    for (auto& mesh : model.meshes)
    {
        texture_indices.albedo = mesh.material.diffuse;
        texture_indices.normal = mesh.material.normal;
        texture_indices.metallic_roughness = mesh.material.metal_roughness;
        texture_indices.ao = mesh.material.ambient_occlusion;
        texture_indices.emissive = mesh.material.emissive;
        pcb.model = pcb.model * mesh.transform;
        pcb.view_position = Vector4(0.0, 0.0, 2.0, 0.0);

        cmd_push_constants(cmd, pbr_pipeline, 0, sizeof(PushConstantBlock), &pcb);
        cmd_push_constants(cmd, pbr_pipeline, sizeof(PushConstantBlock), sizeof(TextureIndices), &texture_indices);
        cmd_bind_vertex_buffer(cmd, mesh.vertex_buffer);
        cmd_bind_index_buffer_u32(cmd, mesh.index_buffer);
        cmd_draw_indexed(cmd, mesh.indices.size(), 1, 0, 0, 0);
    }
}

void on_init()
{
    app_set_shaders_directory("../../../examples/shaders/05_pbr");
    app_set_textures_directory("../../../examples/textures");
    app_set_models_directory("../../../examples/models");

    RendererDesc renderer_desc{};
    renderer_desc.vulkan_allocator = nullptr;
    renderer = create_renderer(renderer_desc);

    DeviceDesc device_desc{};
    device_desc.frame_in_use_count = 2;
    device = create_device(renderer, device_desc);

    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;
    queue = get_queue(device, queue_desc);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = &queue;
    command_pool = create_command_pool(device, command_pool_desc);

    allocate_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);

    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        image_available_semaphores[ i ] = create_semaphore(device);
        rendering_finished_semaphores[ i ] = create_semaphore(device);
        in_flight_fences[ i ] = create_fence(device);
        command_buffers_recorded[ i ] = false;
    }

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width = window_get_width(get_app_window());
    swapchain_desc.height = window_get_height(get_app_window());
    swapchain_desc.queue = &queue;
    swapchain_desc.image_count = FRAME_COUNT;
    swapchain_desc.builtin_depth = true;

    swapchain = create_swapchain(renderer, device, swapchain_desc);

    compute_pbr_maps();

    ubo.projection = create_perspective_matrix(radians(45.0f), window_get_aspect(get_app_window()), 0.1f, 100.f);
    ubo.view = create_look_at_matrix(Vector3(0.0f, 0.0, 2.0f), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));

    BufferDesc ubo_buffer_desc{};
    ubo_buffer_desc.data = &ubo;
    ubo_buffer_desc.size = sizeof(CameraUBO);
    ubo_buffer_desc.resource_state = ResourceState::eTransferSrc | ResourceState::eTransferDst;
    ubo_buffer_desc.descriptor_type = DescriptorType::eUniformBuffer;

    ubo_buffer = create_buffer(device, ubo_buffer_desc);

    load_skybox();
    load_model();
}

void on_resize(u32 width, u32 height)
{
    queue_wait_idle(queue);
    destroy_swapchain(device, swapchain);

    SwapchainDesc swapchain_desc{};
    swapchain_desc.width = window_get_width(get_app_window());
    swapchain_desc.height = window_get_height(get_app_window());
    swapchain_desc.queue = &queue;
    swapchain_desc.image_count = FRAME_COUNT;
    swapchain_desc.builtin_depth = true;

    swapchain = create_swapchain(renderer, device, swapchain_desc);

    ubo.projection = create_perspective_matrix(radians(45.0f), window_get_aspect(get_app_window()), 0.1f, 100.f);
    ubo.view = create_look_at_matrix(Vector3(0.0f, 0.0, 2.0f), Vector3(0.0, 0.0, -1.0), Vector3(0.0, 1.0, 0.0));

    BufferUpdateDesc buffer_update{};
    buffer_update.buffer = &ubo_buffer;
    buffer_update.offset = 0;
    buffer_update.size = sizeof(CameraUBO);
    buffer_update.data = &ubo;

    update_buffer(device, buffer_update);
}

void on_update(f64 delta_time)
{
    u32 image_index = 0;

    begin_frame(image_index);

    auto& cmd = command_buffers[ frame_index ];

    begin_command_buffer(cmd);

    ImageBarrier to_clear_barrier{};
    to_clear_barrier.src_queue = &queue;
    to_clear_barrier.dst_queue = &queue;
    to_clear_barrier.image = &swapchain.images[ image_index ];
    to_clear_barrier.old_state = ResourceState::eUndefined;
    to_clear_barrier.new_state = ResourceState::eColorAttachment;

    cmd_barrier(cmd, 0, nullptr, 1, &to_clear_barrier);

    RenderPassBeginDesc render_pass_begin_desc{};
    render_pass_begin_desc.render_pass = get_swapchain_render_pass(swapchain, image_index);
    render_pass_begin_desc.clear_values[ 0 ].color[ 0 ] = 1.0f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 1 ] = 0.8f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 2 ] = 0.4f;
    render_pass_begin_desc.clear_values[ 0 ].color[ 3 ] = 1.0f;

    render_pass_begin_desc.clear_values[ 1 ].depth = 1.0f;
    render_pass_begin_desc.clear_values[ 1 ].stencil = 0.0;

    cmd_begin_render_pass(cmd, render_pass_begin_desc);
    cmd_set_viewport(cmd, 0, 0, swapchain.width, swapchain.height, 0.0f, 1.0f);
    cmd_set_scissor(cmd, 0, 0, swapchain.width, swapchain.height);
    draw_skybox(cmd);
    draw_model(cmd);
    cmd_end_render_pass(cmd);

    ImageBarrier to_present_barrier{};
    to_present_barrier.src_queue = &queue;
    to_present_barrier.dst_queue = &queue;
    to_present_barrier.image = &swapchain.images[ image_index ];
    to_present_barrier.old_state = ResourceState::eColorAttachment;
    to_present_barrier.new_state = ResourceState::ePresent;

    cmd_barrier(cmd, 0, nullptr, 1, &to_present_barrier);

    end_command_buffer(cmd);

    end_frame(image_index);
}

void on_shutdown()
{
    device_wait_idle(device);
    destroy_pbr_maps();
    release_skybox();
    release_model();
    destroy_buffer(device, ubo_buffer);
    destroy_swapchain(device, swapchain);
    for (u32 i = 0; i < FRAME_COUNT; ++i)
    {
        destroy_semaphore(device, image_available_semaphores[ i ]);
        destroy_semaphore(device, rendering_finished_semaphores[ i ]);
        destroy_fence(device, in_flight_fences[ i ]);
    }
    free_command_buffers(device, command_pool, FRAME_COUNT, command_buffers);
    destroy_command_pool(device, command_pool);
    destroy_device(device);
    destroy_renderer(renderer);
}

int main(int argc, char** argv)
{
    ApplicationConfig config;
    config.argc = argc;
    config.argv = argv;
    config.title = "TestApp";
    config.x = 0;
    config.y = 0;
    config.width = 800;
    config.height = 600;
    config.log_level = LogLevel::eTrace;
    config.on_init = on_init;
    config.on_update = on_update;
    config.on_resize = on_resize;
    config.on_shutdown = on_shutdown;

    app_init(&config);
    app_run();
    app_shutdown();

    return 0;
}
