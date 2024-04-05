# CMakeList.txt : CMake project for ParticleSimulator, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
find_package(OpenGL REQUIRED)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)


# Add source to this project's executable.
add_executable (ParticleSim-Server "ParticleSim-Server.cpp" "glad.c" "Particle.h" "Ghost.h" "ParticleBatch.cpp" "Particle.cpp" "Ghost.cpp")

target_include_directories(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui
	${PROJECT_SOURCE_DIR}/ParticleSim-Server/Libraries/include
    main PUBLIC ${nlohmann_json_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    OpenGL::GL 
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/Libraries/lib/glfw3.lib
    ws2_32
    nlohmann_json::nlohmann_json
)

target_sources(${PROJECT_NAME} PRIVATE 
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui/imgui_impl_opengl3.cpp
    ${PROJECT_SOURCE_DIR}/ParticleSim-Server/imgui/imgui_widgets.cpp
)
message(${PROJECT_SOURCE_DIR}/ParticleSim-Server/include)
# TODO: Add tests and install targets if needed.


set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4099")
