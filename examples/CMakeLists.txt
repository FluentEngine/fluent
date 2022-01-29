function(compile_shaders ${SHADERS})
    foreach(SHADER ${SHADERS})
        add_custom_command(OUTPUT ${SHADER}.spv
        COMMAND glslangValidator -V ${SHADER} -o ${SHADER}.spv
        DEPENDS ${SHADER}
        COMMENT "Rebuilding ${SHADER}.spv" )
        message(STATUS "Generating build commands for ${SHADER}.spv")
    endforeach()
endfunction()

add_subdirectory(01_hello_triangle)
# add_subdirectory(02_load_texture)
# add_subdirectory(03_compute)
# add_subdirectory(04_model_loading)
# add_subdirectory(05_pbr)
