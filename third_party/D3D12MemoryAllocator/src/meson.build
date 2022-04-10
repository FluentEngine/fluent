  d3d12ma_library_source_files = ['D3D12MemAlloc.cpp', '${PROJECT_SOURCE_DIR}/include/D3D12MemAlloc.h']
  if 'WIN32', 'AND', cmake_generator, 'MATCHES', 'Visual Studio.*'
    d3d12ma_library_source_files = [d3d12ma_library_source_files, 'D3D12MemAlloc.natvis']
  endif
  cmake_debug_postfix = 'd'
  cmake_relwithdebinfo_postfix = 'rd'
  cmake_minsizerel_postfix = 's'
  D3D12MemoryAllocator_lib = library('D3D12MemoryAllocator', d3d12ma_library_source_files)
  # set_target_properties(['D3D12MemoryAllocator', 'PROPERTIES', 'CXX_EXTENSIONS', 'OFF', 'CXX_STANDARD', '14', 'CXX_STANDARD_REQUIRED', 'ON', 'OUTPUT_NAME', 'D3D12MA'])
  # target_link_libraries(['D3D12MemoryAllocator', 'PUBLIC', 'd3d12.lib', 'dxgi.lib', 'dxguid.lib'])
  # target_include_directories(['D3D12MemoryAllocator', 'PUBLIC', cmake_current_source_dir, '/../include', 'INTERFACE', '$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>', '$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>'])
  if 'BUILD_SHARED_LIBS'
    # target_compile_definitions(['D3D12MemoryAllocator', 'PRIVATE', 'D3D12MA_EXPORTS'])
    # target_compile_definitions(['D3D12MemoryAllocator', 'INTERFACE', 'D3D12MA_IMPORTS'])
  endif
  # install(['TARGETS', 'D3D12MemoryAllocator', 'DESTINATION', 'lib'])
  # install(['FILES', '${PROJECT_SOURCE_DIR}/include/D3D12MemAlloc.h', 'DESTINATION', 'include'])
  if 'D3D12MA_BUILD_SAMPLE'
    if 'WIN32'
      shader_dir = 'Shaders'
      d3d12_sample_source_files = ['Common.cpp', 'Common.h', 'Tests.cpp', 'Tests.h', 'D3D12Sample.cpp']
      vertex_shaders = '${SHADER_DIR}/VS.hlsl'
      pixel_shaders = '${SHADER_DIR}/PS.hlsl'
      shaders = [vertex_shaders, pixel_shaders]
      # source_group(['Resources\\Shaders', 'FILES', shaders])
      # set_source_files_properties([vertex_shaders, 'PROPERTIES', 'VS_SHADER_TYPE', 'Vertex', 'VS_SETTINGS', 'ExcludedFromBuild=true'])
      # set_source_files_properties([pixel_shaders, 'PROPERTIES', 'VS_SHADER_TYPE', 'Pixel', 'VS_SETTINGS', 'ExcludedFromBuild=true'])
      D3D12Sample_exe = executable('D3D12Sample', d3d12_sample_source_files, shaders)
      # add_dependencies(['D3D12Sample', 'D3D12MemoryAllocator'])
      # Visual Studio specific settings
      if cmake_generator, 'MATCHES', 'Visual Studio.*'
        # Use Unicode instead of multibyte set
        # add_compile_definitions(['UNICODE', '_UNICODE'])
        # Set VmaSample as startup project
        # set_property(['DIRECTORY', project_source_dir, 'PROPERTY', 'VS_STARTUP_PROJECT', 'D3D12Sample'])
        # Enable multithreaded compiling
        # target_compile_options(['D3D12Sample', 'PRIVATE', '/MP'])
        # Set working directory for Visual Studio debugger
        # set_target_properties(['D3D12Sample', 'PROPERTIES', 'VS_DEBUGGER_WORKING_DIRECTORY', '${PROJECT_SOURCE_DIR}/bin'])
      endif
      # set_target_properties(['D3D12Sample', 'PROPERTIES', 'CXX_EXTENSIONS', 'OFF', 'CXX_STANDARD', '14', 'CXX_STANDARD_REQUIRED', 'ON'])
      # target_link_libraries(['D3D12Sample', 'PRIVATE', 'D3D12MemoryAllocator', 'PUBLIC', 'd3d12.lib', 'PUBLIC', 'dxgi.lib', 'PUBLIC', 'dxguid.lib', 'PUBLIC', 'Shlwapi.lib'])
    else
      # message(['STATUS', 'D3D12Sample application is not supported to Linux'])
    endif
  endif
