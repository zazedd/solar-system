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

add_compile_definitions(WITH_MINIAUDIO)

include_directories(..)
include_directories(libs)
include_directories(libs/SOIL2)
include_directories(libs/imgui)
include_directories(libs/soloud)
include_directories(libs/soloud/coreaudio)
include_directories(libs/soloud/wav)
include_directories(planet)
include_directories(resources)
if (APPLE)
        set(ASSIMP assimp::assimp)
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

file(GLOB imgui
     "libs/imgui/*.h"
     "libs/imgui/*.cpp"
)

file(GLOB soil2
     "libs/SOIL2/*.h"
     "libs/SOIL2/*.c"
)

file(GLOB soloud
     "libs/soloud/*.h"
     "libs/soloud/*.cpp"
)

file(GLOB soloudBackend
     "libs/soloud/miniaudio/*.h"
     "libs/soloud/miniaudio/*.cpp"
)

file(GLOB soloudWav
     "libs/soloud/wav/*.h"
     "libs/soloud/wav/*.cpp"
)

add_executable(solarsystem
  main.cpp
  planet/camera.h
  planet/mesh.h
  planet/model.h
  planet/planet.hpp
  planet/planet.cpp
  planet/shader.h
  planet/audio.cpp
  planet/audio.h

  libs/stb_vorbis.c

  ${imgui}
  ${soil2}
  ${soloudWav}
  ${soloudBackend}
  ${soloud}
)

target_link_libraries(solarsystem ${ALL_LIBS})

if (APPLE)
  target_link_libraries(solarsystem "-framework Cocoa")
  target_link_libraries(solarsystem "-framework OpenGL")
  target_link_libraries(solarsystem "-framework IOKit")
endif()

