cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
# Using folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(voxel-game)

# Default compile flags
if(MSVC)
	set(PEDANTIC_COMPILE_FLAGS /W3)
else()
	if(UNIX)
		# Extra libraries for linux
		set(LINK_LIBS ${CMAKE_DL_LIBS} GL pthread Xi X11)
	endif()

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -fno-omit-frame-pointer -O0")
	set(PEDANTIC_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wconversion -Wundef ")
endif()

# Compiler warning flags from FMT
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	string(CONCAT PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}"
		"-Wold-style-cast -Wredundant-decls -Wwrite-strings -Wpointer-arith "
		"-Wcast-qual -Wformat=2 -Wmissing-include-dirs -Wcast-align -Winvalid-pch "
		"-Wctor-dtor-privacy -Wdisabled-optimization -Woverloaded-virtual "
		"-Wno-ctor-dtor-privacy -Wno-format-nonliteral "
	)

	if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
		set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wno-dangling-else -Wno-unused-local-typedefs ")
	endif()

	if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
		string(CONCAT PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS}"
			"-Wdouble-promotion -Wtrampolines -Wzero-as-null-pointer-constant -Wuseless-cast "
			"-Wvector-operation-performance -Wsized-deallocation -Wshadow "
		)
	endif()
	if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
		set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2 -Wduplicated-cond ")
		if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12.0)
			set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wnull-dereference ")
		endif()
	endif()
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wshadow -Wdeprecated -Wweak-vtables -Wno-gnu-zero-variadic-macro-arguments ")
	check_cxx_compiler_flag(-Wzero-as-null-pointer-constant HAS_NULLPTR_WARNING)
	if(HAS_NULLPTR_WARNING)
		set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wzero-as-null-pointer-constant ")
	endif()
endif()

# Set determined compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PEDANTIC_COMPILE_FLAGS}")

# Find OpenGL package
find_package(OpenGL REQUIRED)

# GLFW CMake settings
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE) # Prefer X11 over Wayland for Linux
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# fmt CMake settings
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(FMT_OS OFF CACHE BOOL "" FORCE)

# Get external libraries
include(FetchContent)
set(LIB_DIR "${PROJECT_SOURCE_DIR}/libraries/")

if (NOT EXISTS ${LIB_DIR}/lodepng/lodepng.h)
	FetchContent_Declare(
		ext_lodepng
		GIT_REPOSITORY https://github.com/lvandeve/lodepng
		GIT_TAG master
		SOURCE_DIR "${LIB_DIR}/lodepng"
	)
	FetchContent_MakeAvailable(ext_lodepng)
endif()
if (NOT EXISTS ${LIB_DIR}/fmt/include/base.h)
	FetchContent_Declare(
		ext_fmt
		GIT_REPOSITORY https://github.com/fmtlib/fmt
		GIT_TAG master
		SOURCE_DIR "${LIB_DIR}/fmt"
	)
	FetchContent_MakeAvailable(ext_fmt)
else()
	add_subdirectory(${LIB_DIR}/fmt)
endif()
if (NOT EXISTS ${LIB_DIR}/glfw/include/GLFW/glfw3.h)
	FetchContent_Declare(
		ext_glfw
		GIT_REPOSITORY https://github.com/glfw/glfw
		GIT_TAG master
		SOURCE_DIR "${LIB_DIR}/glfw"
		FIND_PACKAGE_ARGS NAMES glfw3
	)
	FetchContent_MakeAvailable(ext_glfw)
else()
	add_subdirectory(${LIB_DIR}/glfw)
endif()

if(EXISTS ${LIB_DIR}/lodepng/lodepng.cpp)
	file(RENAME ${LIB_DIR}/lodepng/lodepng.cpp ${LIB_DIR}/lodepng/lodepng.c)
endif()

# Folder shortcuts
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)

set(SC_W  ${SRC_DIR}/World)
set(SC_WG ${SRC_DIR}/World/Generation)
set(SC_P  ${SRC_DIR}/Player)
set(SC_R  ${SRC_DIR}/Rendering)
set(SC_A  ${SRC_DIR}/Application)

# Create executable with required C/C++ source files
add_executable(
	# Initial project argument
	${PROJECT_NAME}

	# External libraries
	# lodepng
	${LIB_DIR}/lodepng/lodepng.c
	# glad
	${LIB_DIR}/glad/voxel_glad.c

	# Main game
	# src/
	${SRC_DIR}/Main.cpp
		# src/Player
		${SC_P}/Player.cpp
		# src/Rendering
		${SC_R}/TextRenderer.cpp
		${SC_R}/Frustum.cpp
		# src/Application
		${SC_A}/Definitions.cpp
		${SC_A}/Callbacks.cpp
		${SC_A}/Game.cpp
		# src/World
		${SC_W}/Chunk.cpp
		${SC_W}/Sky.cpp
		${SC_W}/World.cpp
			# src/World/Generation
			${SC_WG}/Perlin.cpp
			${SC_WG}/Settings.cpp
			${SC_WG}/Structures.cpp
)

# Directories for "#include ..."
target_include_directories(
	${PROJECT_NAME} PUBLIC
	${LIB_DIR} # Libraries root folder
	${SRC_DIR} # Game source folder
)

# External libraries to link
target_link_libraries(
	${PROJECT_NAME} PUBLIC
	${LINK_LIBS}
	glfw
	fmt
)

# Shortcut to resulting directory
set(RESULT_DIR ${PROJECT_SOURCE_DIR}/game)

# Specify directory with executable results as defined above
set_target_properties(
	${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${RESULT_DIR}
)

# Add resources (shaders and textures) to resulting directory
file(COPY ${SRC_DIR}/Resources DESTINATION ${RESULT_DIR})

# Set C/C++ standards
set_target_properties(
	${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 11
	CXX_EXTENSIONS NO
	C_EXTENSIONS NO
)
