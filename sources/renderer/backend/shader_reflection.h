#pragma once

#include "base/base.h"
#include "renderer/backend/renderer_enums.h"

int32_t
binding_map_compare( const void* a, const void* b, void* udata );

uint64_t
binding_map_hash( const void* item, uint64_t seed0, uint64_t seed1 );

void
dxil_reflect( const struct ft_device*      device,
              const struct ft_shader_info* info,
              struct ft_shader*            shader );

void
spirv_reflect( const struct ft_device*      device,
               const struct ft_shader_info* info,
               struct ft_shader*            shader );

void
mtl_reflect( const struct ft_device*      device,
             const struct ft_shader_info* info,
             struct ft_shader*            shader );
