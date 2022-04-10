  project('D3D12MemoryAllocator', default_options : ['default_library=static'])
  # set_property(['GLOBAL', 'PROPERTY', 'USE_FOLDERS', 'ON'])
  Doxygen_dep = dependency('Doxygen')
  if 'BUILD_DOCUMENTATION'
    if 'DOXYGEN_FOUND'
      # set input and output files
      doxygen_in = [cmake_current_source_dir, '/src/Doxyfile']
      doxygen_out = [cmake_current_binary_dir, '/Doxyfile']
      # request to configure the file
      # configure_file([doxygen_in, doxygen_out, '@ONLY'])
      # note the option ALL which allows to build the docs together with the application
      # add_custom_target(['doc_doxygen', 'ALL', 'COMMAND', doxygen_executable, doxygen_out, 'WORKING_DIRECTORY', cmake_current_binary_dir, 'COMMENT', 'Generating API documentation with Doxygen', 'VERBATIM'])
    else
      # message('Doxygen need to be installed to generate the doxygen documentation')
    endif
  endif
  # D3D12MemoryAllocator contains an sample application which is not build by default
  # message(['STATUS', 'D3D12MA_BUILD_SAMPLE = ${D3D12MA_BUILD_SAMPLE}'])
  # include_directories('include')
  subdir('src')
