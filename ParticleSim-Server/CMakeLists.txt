# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
project("ParticleSim-Server")

add_subdirectory("ParticleSim-Server")
find_package(OpenGL REQUIRED)



