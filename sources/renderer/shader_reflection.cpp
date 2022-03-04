#include <spirv_reflect.h>
#include "renderer/shader_reflection.hpp"

namespace fluent
{

ShaderType get_type_by_reflection(const SpvReflectTypeDescription& type)
{
    Format format = Format::eUndefined;
    if (type.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
		if (type.traits.numeric.vector.component_count < 2 ||
		    type.traits.numeric.matrix.column_count < 2)
            format = Format::eR32Sfloat;
		if (type.traits.numeric.vector.component_count == 2 ||
		    type.traits.numeric.matrix.column_count == 2)
            format = Format::eR32G32Sfloat;
		if (type.traits.numeric.vector.component_count == 3 ||
		    type.traits.numeric.matrix.column_count == 3)
            format = Format::eR32G32B32Sfloat;
		if (type.traits.numeric.vector.component_count == 4 ||
		    type.traits.numeric.matrix.column_count == 4)
            format = Format::eR32G32B32A32Sfloat;
    }
    if (type.type_flags & SPV_REFLECT_TYPE_FLAG_INT)
    {
		if (type.traits.numeric.vector.component_count < 2 ||
		    type.traits.numeric.matrix.column_count < 2)
			format = type.traits.numeric.scalar.signedness ? Format::eR32Sint
			                                               : Format::eR32Uint;
		if (type.traits.numeric.vector.component_count == 2 ||
		    type.traits.numeric.matrix.column_count == 2)
			format = type.traits.numeric.scalar.signedness
			             ? Format::eR32G32Sint
			             : Format::eR32G32Uint;
		if (type.traits.numeric.vector.component_count == 3 ||
		    type.traits.numeric.matrix.column_count == 3)
			format = type.traits.numeric.scalar.signedness
			             ? Format::eR32G32B32Sint
			             : Format::eR32G32B32Uint;
		if (type.traits.numeric.vector.component_count == 4 ||
		    type.traits.numeric.matrix.column_count == 4)
			format = type.traits.numeric.scalar.signedness
			             ? Format::eR32G32B32A32Sint
			             : Format::eR32G32B32A32Uint;
    }

    FT_ASSERT(format != Format::eUndefined);

    u32 byte_size       = type.traits.numeric.scalar.width / 8;
    u32 component_count = 1;

    if (type.traits.numeric.vector.component_count > 0)
        byte_size *= type.traits.numeric.vector.component_count;
    else if (type.traits.numeric.matrix.row_count > 0)
        byte_size *= type.traits.numeric.matrix.row_count;

    if (type.traits.numeric.matrix.column_count > 0)
        component_count = type.traits.numeric.matrix.column_count;

    return ShaderType{ format, component_count, byte_size };
}

void recursive_uniform_visit(
    std::vector<ShaderType>&         uniform_variables,
    const SpvReflectTypeDescription& type)
{
    if (type.member_count > 0)
    {
        for (uint32_t i = 0; i < type.member_count; i++)
            recursive_uniform_visit(uniform_variables, type.members[ i ]);
    }
    else
    {
		if (type.type_flags &
		    (SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_FLOAT))
            uniform_variables.push_back(get_type_by_reflection(type));
    }
}

DescriptorType to_desctriptor_type(VkDescriptorType descriptor_type)
{
    switch (descriptor_type)
    {
    case VK_DESCRIPTOR_TYPE_SAMPLER:
        return DescriptorType::eSampler;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        FT_ASSERT(false && "Use separate types instead, texture + sampler");
        return DescriptorType(-1);
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return DescriptorType::eSampledImage;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return DescriptorType::eStorageImage;
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return DescriptorType::eUniformTexelBuffer;
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return DescriptorType::eStorageTexelBuffer;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return DescriptorType::eUniformBuffer;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return DescriptorType::eStorageBuffer;
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return DescriptorType::eUniformBufferDynamic;
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return DescriptorType::eStorageBufferDynamic;
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return DescriptorType::eInputAttachment;
    default:
        FT_ASSERT(false);
        return DescriptorType(-1);
    }
}

ReflectionData reflect(u32 byte_code_size, const u32* byte_code)
{
    ReflectionData result{};

    SpvReflectResult       spv_result;
    SpvReflectShaderModule reflected_shader;
	spv_result = spvReflectCreateShaderModule(
	    byte_code_size, ( const void* ) byte_code, &reflected_shader);
    FT_ASSERT(spv_result == SPV_REFLECT_RESULT_SUCCESS);

    u32 descriptor_binding_count = 0;
	spv_result                   = spvReflectEnumerateDescriptorBindings(
	    &reflected_shader, &descriptor_binding_count, nullptr);
    FT_ASSERT(spv_result == SPV_REFLECT_RESULT_SUCCESS);
	std::vector<SpvReflectDescriptorBinding*> descriptor_bindings(
	    descriptor_binding_count);
	spv_result = spvReflectEnumerateDescriptorBindings(
	    &reflected_shader,
	    &descriptor_binding_count,
	    descriptor_bindings.data());
    FT_ASSERT(spv_result == SPV_REFLECT_RESULT_SUCCESS);

    result.binding_count = descriptor_binding_count;
    result.bindings.resize(result.binding_count);

    u32 i = 0;
    for (const auto& descriptor_binding : descriptor_bindings)
    {
        result.bindings[ i ].binding          = descriptor_binding->binding;
        result.bindings[ i ].descriptor_count = descriptor_binding->count;
		result.bindings[ i ].descriptor_type  = to_desctriptor_type(
		    ( VkDescriptorType ) descriptor_binding->descriptor_type);
        result.bindings[ i ].set = descriptor_binding->set;
        i++;
    }

    spvReflectDestroyShaderModule(&reflected_shader);

    return result;
}
} // namespace fluent
