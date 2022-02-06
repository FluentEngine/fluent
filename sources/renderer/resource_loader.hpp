#pragma once

#include "renderer/renderer_backend.hpp"

namespace fluent
{
class ResourceLoader
{
private:
    static const Device*  m_device;
    static Queue*         m_queue;
    static CommandPool*   m_command_pool;
    static CommandBuffer* m_cmd;

    static Buffer* create_staging_buffer(u64 size, const void* data);

public:
    static void init(const Device* m_device);
    static void shutdown();
    static void upload_buffer(Buffer* buffer, u64 offset, u64 size, const void* data);
    static void upload_image(Image* image, u64 size, const void* data);
};
} // namespace fluent
