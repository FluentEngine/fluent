#pragma once

#ifdef VULKAN_BACKEND
#include "renderer/renderer_backend.hpp"

namespace fluent
{
void vk_create_renderer_backend( const RendererBackendDesc* desc,
                                 RendererBackend**          p_backend );
} // namespace fluent
#endif
