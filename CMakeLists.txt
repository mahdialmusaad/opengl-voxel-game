cmake_minimum_required(VERSION 3.10)
project(game)

# Optionally set C++ version
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# GLFW build options
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

# Compiler warning settings
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
	# Define glad library
	if(NOT WIN32)
		set(GLAD_LIBRARIES dl)
		set(GL_FLAG -lGL)
	endif()

	# Debug build settings
	set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Og -g3 -fno-inline -ggdb")

	# Normal build settings
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic ${GL_FLAG}")
endif()

# Shortcut to libraries directory in root and game source directory
set(BCL ${PROJECT_SOURCE_DIR}/libraries)
set(BC_SRC ${PROJECT_SOURCE_DIR}/src)

# Include folders, used for '#include ...'
include_directories(
	${BCL} # Libraries root folder
	${BC_SRC} # Game source folder
)

# Add GLFW to project
add_subdirectory(${BCL}/glfw/)

# Folder shortcuts
set(BCS_W ${BC_SRC}/World)
set(BCS_WG ${BC_SRC}/World/Generation)
set(BCS_P ${BC_SRC}/Player)
set(BCS_R ${BC_SRC}/Rendering)
set(BCS_A ${BC_SRC}/Application)

# Source files
add_executable(
	# Initial project argument
	${PROJECT_NAME}

	# External libraries
	# lodepng
	${BCL}/lodepng/lodepng.cpp
	# glad
	${BCL}/glad/gladc.c

	# Main game
	# src/
	${BC_SRC}/Main.cpp
		# src/Player
		${BCS_P}/Player.cpp
		# src/Rendering
		${BCS_R}/TextRenderer.cpp
		${BCS_R}/Frustum.cpp
		# src/Application
		${BCS_A}/Definitions.cpp
		${BCS_A}/Callbacks.cpp
		${BCS_A}/Game.cpp
		# src/World
		${BCS_W}/Chunk.cpp
		${BCS_W}/Sky.cpp
		${BCS_W}/World.cpp
			# src/World/Generation
			${BCS_WG}/Perlin.cpp
			${BCS_WG}/Settings.cpp
)

# Link external libraries
target_link_libraries(
	${PROJECT_NAME} 
	glfw
	${GLAD_LIBRARIES}
)

# Shortcut to resulting folder
set(RESULT_FLD ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

# Specify folder with build results as defined above
set_target_properties(
	${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${RESULT_FLD}
)

# Add resources (shaders and textures) to resulting folder
message("-- Copying game resources")
file(COPY ${BC_SRC}/Resources DESTINATION ${RESULT_FLD})

# CTest
include(CTest)
enable_testing()

# Valgrind testing
find_program(VALGRIND "valgrind")
if(VALGRIND)
	# Valgrind leak check test
	add_test(
		NAME valgrind_memcheck
        	COMMAND ${VALGRIND}
		  --leak-check=full
		  --tool=memcheck
		$<TARGET_FILE:${PROJECT_NAME}>
	)
endif()
