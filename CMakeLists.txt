# CMake entry point
cmake_minimum_required (VERSION 3.0)
project (SolarSystem)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
if (APPLE)
        find_package(assimp REQUIRED)
endif()
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
include_directories(libs/SOIL2)
include_directories(libs/imgui)
include_directories(planet)
include_directories(resources)
if (APPLE)
        set(ASSIMP assimp::assimp)
        message("test")
else()
        include_directories(/usr/include/assimp)
        set(ASSIMP assimp.so)
endif()

set(ALL_LIBS
 ${OPENGL_LIBRARY}
 glfw
 glm::glm
 ${ASSIMP}
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
  libs/imgui/imgui.h
  libs/imgui/imgui.cpp
  libs/imgui/imgui_draw.cpp
  libs/imgui/imgui_internal.h
  libs/imgui/imgui_tables.cpp
  libs/imgui/imgui_widgets.cpp
  libs/imgui/imstb_rectpack.h
  libs/imgui/imstb_textedit.h
  libs/imgui/imstb_truetype.h
  libs/imgui/imgui_impl_glfw.h
  libs/imgui/imgui_impl_glfw.cpp
  libs/imgui/imgui_impl_opengl3.h
  libs/imgui/imgui_impl_opengl3.cpp
  libs/imgui/imgui_impl_opengl3_loader.h
  libs/imgui/imconfig.h
  libs/SOIL2/SOIL2.c
  libs/SOIL2/SOIL2.h
  libs/SOIL2/stb_image.h
  libs/SOIL2/jo_jpeg.h
  libs/SOIL2/stbi_DDS.h
  libs/SOIL2/stbi_ext.h
  libs/SOIL2/stbi_pkm.h
  libs/SOIL2/stbi_pvr.h
  libs/SOIL2/image_DXT.c
  libs/SOIL2/image_DXT.h
  libs/SOIL2/etc1_utils.c
  libs/SOIL2/etc1_utils.h
  libs/SOIL2/pkm_helper.h
  libs/SOIL2/pvr_helper.h
  libs/SOIL2/stbi_DDS_c.h
  libs/SOIL2/stbi_ext_c.h
  libs/SOIL2/stbi_pkm_c.h
  libs/SOIL2/stbi_pvr_c.h
  libs/SOIL2/image_helper.c
  libs/SOIL2/image_helper.h
  libs/SOIL2/stb_image_write.h
  planet/camera.h
  planet/mesh.h
  planet/model.h
  planet/planet.hpp
  planet/planet.cpp
  planet/shader.h


  resources/shaders/lamp.vs
  resources/shaders/lamp.frag
  resources/shaders/modelLoading.vs
  resources/shaders/modelLoading.frag
  resources/shaders/earth.vs
  resources/shaders/earth.frag
  resources/shaders/path.vs
  resources/shaders/path.frag

)

target_link_libraries(solarsystem ${ALL_LIBS})

if (APPLE)
  target_link_libraries(solarsystem "-framework Cocoa")
  target_link_libraries(solarsystem "-framework OpenGL")
  target_link_libraries(solarsystem "-framework IOKit")
endif()

