# CMake entry point
cmake_minimum_required (VERSION 3.0)
project (SolarSystem)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(GLEW REQUIRED)

if( CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR )
  message( FATAL_ERROR "Please select another Build Directory ! (and give it a clever name, like bin_Visual2012_64bits/)" )
endif()
if( CMAKE_SOURCE_DIR MATCHES " " )
  message( "Your Source Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()
if( CMAKE_BINARY_DIR MATCHES " " )
  message( "Your Build Directory contains spaces. If you experience problems when compiling, this can be the cause." )
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(..)
include_directories(SOIL2)
include_directories(planet)
include_directories(resources)

set(ALL_LIBS
 ${OPENGL_LIBRARY}
 glfw
 glm::glm
 assimp::assimp
 GLEW::glew
)

add_definitions(
 -DTW_STATIC
 -DTW_NO_LIB_PRAGMA
 -DTW_NO_DIRECT3D
 -DGLEW_STATIC
 -D_CRT_SECURE_NO_WARNINGS
)

add_executable(solarsystem
  main.cpp
  SOIL2/SOIL2.c
  SOIL2/SOIL2.h
  SOIL2/stb_image.h
  SOIL2/jo_jpeg.h
  SOIL2/stbi_DDS.h
  SOIL2/stbi_ext.h
  SOIL2/stbi_pkm.h
  SOIL2/stbi_pvr.h
  SOIL2/image_DXT.c
  SOIL2/image_DXT.h
  SOIL2/etc1_utils.c
  SOIL2/etc1_utils.h
  SOIL2/pkm_helper.h
  SOIL2/pvr_helper.h
  SOIL2/stbi_DDS_c.h
  SOIL2/stbi_ext_c.h
  SOIL2/stbi_pkm_c.h
  SOIL2/stbi_pvr_c.h
  SOIL2/image_helper.c
  SOIL2/image_helper.h
  SOIL2/stb_image_write.h
  planet/camera.h
  planet/mesh.h
  planet/model.h
  planet/planet.hpp
  planet/planet.cpp
  planet/shader.h


  resources/shaders/lamp.vs
  resources/shaders/lamp.frag
  resources/shaders/directional.vs
  resources/shaders/directional.frag
  resources/shaders/modelLoading.vs
  resources/shaders/modelLoading.frag
  
)

target_link_libraries(solarsystem ${ALL_LIBS})

if (APPLE)
  target_link_libraries(solarsystem "-framework Cocoa")
  target_link_libraries(solarsystem "-framework OpenGL")
  target_link_libraries(solarsystem "-framework IOKit")
endif()
