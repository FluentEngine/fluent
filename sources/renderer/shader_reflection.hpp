#pragma once

#include <vector>
#include "core/base.hpp"
#include "renderer/renderer_enums.hpp"

namespace fluent
{
struct ShaderType
{
    Format format;
    u32    component_count;
    u32    byte_size;
};

struct Binding
{
    u32            set;
    u32            binding;
    u32            descriptor_count;
    DescriptorType descriptor_type;
};

struct ReflectionData
{
    u32                  binding_count;
    std::vector<Binding> bindings;
};

ReflectionData
spirv_reflect( u32 byte_code_size, const void* byte_code );
ReflectionData
dxil_reflect( u32 byte_code_size, const void* byte_code );

} // namespace fluent
