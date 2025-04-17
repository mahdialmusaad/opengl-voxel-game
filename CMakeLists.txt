cmake_minimum_required(VERSION 3.10.0)
set (CMAKE_CXX_STANDARD 23)
project(badcraft VERSION 1.0.0 LANGUAGES C CXX)
set(BC_LIBS "${PROJECT_SOURCE_DIR}/libraries")
set(BC_SRC "${PROJECT_SOURCE_DIR}/src")
include_directories("${BC_LIBS}")
include_directories("${BC_SRC}")
link_directories("${BC_LIBS}/glfwlib")
file(COPY "${BC_SRC}/Resources" DESTINATION ${CMAKE_BINARY_DIR}/src)
set(BCS "${BC_SRC}/")
set(BCL "${BC_LIBS}/")
add_executable(badcraft
    ${BCS}Main.cpp
    ${BCS}Player/Player.cpp
    ${BCS}Rendering/TextRenderer.cpp
    ${BCS}World/Chunk.cpp
    ${BCS}Utility/Application.cpp
    ${BCS}Utility/Definitions.cpp
    ${BCS}World/Sky.cpp
    ${BCS}World/World.cpp
    ${BCS}World/Generation/Perlin.cpp
    ${BCS}World/Generation/Settings.cpp

    ${BCL}glad/glad.c
    ${BCL}glm/glm.cppm
    ${BCL}lodepng/lodepng.cpp
)
target_link_libraries(badcraft PRIVATE "glfw3.lib")