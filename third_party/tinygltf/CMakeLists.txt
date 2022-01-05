cmake_minimum_required(VERSION 3.6)

PROJECT (tinygltf)

SET(CMAKE_CXX_STANDARD 11)

option(TINYGLTF_HEADER_ONLY "On: header-only mode. Off: create tinygltf library(No TINYGLTF_IMPLEMENTATION required in your project)" ON)

#
# for add_subdirectory and standalone build
#
if (TINYGLTF_HEADER_ONLY)
  add_library(tinygltf INTERFACE)

  target_include_directories(tinygltf
          INTERFACE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

else (TINYGLTF_HEADER_ONLY)
  add_library(tinygltf)
  target_sources(tinygltf PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/tiny_gltf.cc)
  target_include_directories(tinygltf
          INTERFACE
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
          )
  target_link_libraries(tinygltf PUBLIC stb_image)
endif (TINYGLTF_HEADER_ONLY)