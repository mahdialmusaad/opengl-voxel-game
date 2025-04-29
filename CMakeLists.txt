cmake_minimum_required(VERSION 3.10)
project(badcraft)

# GLFW build options for... GLFW!
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

# Compiler warning settings (you need to hold yourselves to higher standards!)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-reorder")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()

# Shortcut to libraries directory in root and game source directory
set(BCL ${PROJECT_SOURCE_DIR}/libraries)
set(BC_SRC ${PROJECT_SOURCE_DIR}/src)

# Include folders, used for '#include ...'
include_directories(
	# Libraries root folder
	${BCL}
	# Game source folder
	${BC_SRC}
	# Some external library folders that would be annoying to write in '#include'
	${BCL}/glfw/include
	${BCL}/glm
	${BCL}/glad
)

# Add GLFW to project
add_subdirectory(${BCL}/glfw)
# Get OpenGL package
find_package(OpenGL REQUIRED)

# Folder shortcuts
set(BCS_W ${BC_SRC}/World)
set(BCS_WG ${BC_SRC}/World/Generation)
set(BCS_P ${BC_SRC}/Player)
set(BCS_R ${BC_SRC}/Rendering)
set(BCS_A ${BC_SRC}/Application)
set(BCS_G ${BC_SRC}/Globals)

# Source files
add_executable(
	# Initial project argument
	${PROJECT_NAME}
	# glad.c
	${BCL}/glad/glad.c
	# lodepng.cpp
	${BCL}/lodepng/lodepng.cpp
	# src/Globals
    ${BCS_G}/Definitions.cpp
	# src/Player
    ${BCS_P}/Player.cpp
	# src/Rendering
    ${BCS_R}/TextRenderer.cpp
	# src/Application
    ${BCS_A}/Application.cpp
	${BCS_A}/Callbacks.cpp
	# src/World
    ${BCS_W}/Chunk.cpp
    ${BCS_W}/Sky.cpp
    ${BCS_W}/World.cpp
	# src/World/Generation
		${BCS_WG}/Perlin.cpp
		${BCS_WG}/Settings.cpp
	# src/
    ${BC_SRC}/Main.cpp
)


# Link external libraries
target_link_libraries(
	${PROJECT_NAME} 
	glfw 
	OpenGL::GL
	${GLAD_LIBRARIES} 
	${GLFW_LIBRARIES}
)

# Shortcut to resulting folder
set(RESULT_FLD ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/)

# Specify folder with build results as defined above
set_target_properties(
	${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${RESULT_FLD}
)

# Add resources (shaders and textures) to resulting folder
file(COPY ${BC_SRC}/Resources DESTINATION ${RESULT_FLD})
