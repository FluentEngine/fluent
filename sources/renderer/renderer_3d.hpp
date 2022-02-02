#pragma once

#include <unordered_map>
#include <list>
#include "renderer/renderer_backend.hpp"
#include "renderer/graphic_context.hpp"
#include "resource_manager/resource_manager.hpp"
#include "scene/camera.hpp"

namespace fluent
{
class Renderer3D
{
private:
    static GraphicContext* m_context;

    struct RendererData
    {
        BaseImage*           output_image          = nullptr;
        BaseImage*           output_depth_image    = nullptr;
        RenderPass*          render_pass           = nullptr;
        DescriptorSetLayout* descriptor_set_layout = nullptr;
        Pipeline*            pipeline              = nullptr;
        DescriptorSet*       per_frame_set         = nullptr;
        DescriptorSet*       per_material_set      = nullptr;
        Ref<Buffer>          uniform_buffer        = nullptr;
        u32                  viewport_width        = 0;
        u32                  viewport_height       = 0;
    };

    struct RendererResources
    {
        Ref<Image> default_albedo;
        Sampler*   default_sampler;
    };

    struct FrameData
    {
        std::list<DescriptorSet*> free_set_lists[ GraphicContext::frame_count() ];
    };

    struct PushConstantBlock
    {
        Matrix4 model;
        Vector4 viewer_position;
    };

    static RendererData      m_data;
    static RendererResources m_resources;
    static FrameData         m_frame_data;
    static PushConstantBlock m_push_constant_block;

    static void create_output_images(u32 width, u32 height);
    static void destroy_output_images();
    static void create_render_pass(u32 width, u32 height);
    static void setup_vertex_layout(VertexLayout& vertex_layout);
    static void create_default_resources();
    static void destroy_default_resources();

public:
    static void init(u32 width, u32 height);
    static void shutdown();
    static void begin_frame(const Camera& camera);
    static void draw_geometry(const Matrix4& transform, const Ref<Geometry> geometry);
    static void draw_geometry(const Matrix4& transform, const Ref<Geometry> geometry, const Ref<Image> image);
    static void end_frame();

    static void resize_viewport(u32 width, u32 height);

    static BaseImage* get_output_image()
    {
        return m_data.output_image;
    }
};
} // namespace fluent
