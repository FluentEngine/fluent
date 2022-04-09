set(tools_dir "${FLUENT_ROOT_DIRECTORY}/tools")

function(compile_shader type shader_name compiled_shader_name)
    message("Compile: \n" ${shader_name})

    set(res 0)

    if (RENDERER_BACKEND_VULKAN)
        execute_process(COMMAND glslangValidator -e main -V ${shader_name} -o ${compiled_shader_name} OUTPUT_QUIET RESULT_VARIABLE res OUTPUT_VARIABLE out)
    endif()

    if(RENDERER_BACKEND_D3D12)
        execute_process(COMMAND dxc -T ${type} -Fo ${compiled_shader_name} ${shader_name} OUTPUT_QUIET RESULT_VARIABLE res OUTPUT_VARIABLE out)
    endif()

    if (${res} EQUAL "0")
        message("Compiled: \n" ${compiled_shader_name})
    else()
        message("Shader compilation failed" ${out})
    endif()
endfunction()

function (compile_shader_group type shaders)
    foreach(shader IN LISTS shaders)
        string(REGEX REPLACE "hlsl" "bin" compiled_shader_name ${shader})
        compile_shader("${type}" "${shader}" "${compiled_shader_name}")
    endforeach()
endfunction()

function(compile_shaders target_name directory)
    file(GLOB vert ${directory}*vert.hlsl)
    file(GLOB frag ${directory}*frag.hlsl)
    file(GLOB comp ${directory}*comp.hlsl)

    compile_shader_group(vs_6_5 "${vert}")
    compile_shader_group(ps_6_5 "${frag}")
    compile_shader_group(cs_6_5 "${comp}")
endfunction()
