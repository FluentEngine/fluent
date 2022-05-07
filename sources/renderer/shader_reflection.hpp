#pragma once

#include <unordered_map>
#include <vector>
#include "core/base.hpp"
#include "renderer/renderer_enums.hpp"

namespace fluent
{

struct Device;
struct Shader;
struct ShaderInfo;

struct Binding
{
	u32            set;
	u32            binding;
	u32            descriptor_count;
	DescriptorType descriptor_type;
	ShaderStage    stage;
};

using Bindings   = std::vector<Binding>;
using BindingMap = std::unordered_map<std::string, u32>;

struct ReflectionData
{
	u32        binding_count;
	Bindings   bindings;
	BindingMap binding_map;
};

void
dxil_reflect( const Device* device, const ShaderInfo* info, Shader* shader );

void
spirv_reflect( const Device* device, const ShaderInfo* info, Shader* shader );

void
mtl_reflect( const Device* device, const ShaderInfo* info, Shader* shader );

} // namespace fluent
