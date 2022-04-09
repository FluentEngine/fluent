#pragma once

#ifdef D3D12_BACKEND
#include "renderer/renderer_backend.hpp"

namespace fluent
{
void d3d12_create_renderer_backend( const RendererBackendDesc* desc,
                                    RendererBackend**          p_backend );
} // namespace fluent
#endif
