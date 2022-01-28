#pragma once

#include "core/base.hpp"
#include "renderer/renderer.hpp"

namespace fluent
{

// for future use
using UpdateFence = void*;

struct ImageUpdateDesc
{
    Image*        image;
    u32           size;
    const void*   data;
    ResourceState resource_state;
};

struct BufferUpdateDesc
{
    u32         offset;
    u32         size;
    const void* data;
    Buffer*     buffer;
};

void init_resource_loader(Device const& device);
void shutdown_resource_loader(Device const& device);

void resource_loader_wait_idle();

Shader load_shader(const Device& device, const std::string& filename);

void update_image(const Device& device, ImageUpdateDesc const& desc, UpdateFence* fence = nullptr);
void update_buffer(const Device& device, BufferUpdateDesc const& desc, UpdateFence* fence = nullptr);

Image load_image_from_dds_file(const Device& device, const char* filename, b32 flip);
Image load_image_from_file(const Device& device, const char* filename, b32 flip);

} // namespace fluent
