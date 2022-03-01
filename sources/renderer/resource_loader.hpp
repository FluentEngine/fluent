#pragma once

#include "renderer/renderer_backend.hpp"

namespace fluent
{
class ResourceLoader
{
    struct StagingBuffer
    {
        u64     offset = 0;
        Buffer* buffer = nullptr;
    };

private:
    static constexpr u64 STAGING_BUFFER_SIZE = 25 * 1024 * 1024 * 8;

    static const Device*  m_device;
    static Queue*         m_queue;
    static CommandPool*   m_command_pool;
    static CommandBuffer* m_cmd;
    static b32            m_is_recording;
    static StagingBuffer  m_staging_buffer;

public:
    static void init(const Device* m_device);
    static void shutdown();
    static void begin_recording();
    static void end_recording();
    static void upload_buffer(
        Buffer* buffer, u64 offset, u64 size, const void* data);
    static void* begin_upload_buffer(Buffer* buffer);
    static void  end_upload_buffer(Buffer* buffer);
    static void  upload_image(Image* image, u64 size, const void* data);
    static void  reset_staging_buffer();
};
} // namespace fluent
