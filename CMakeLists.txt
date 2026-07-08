cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(game)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/game)
set(VX_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

file(COPY ${PROJECT_SOURCE_DIR}/resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

find_package(glfw3 OPTIONAL)
find_package(ZLIB OPTIONAL)

if (NOT glfw3_FOUND)
	set(GLFW_BUILD_DOCS OFF)

	include(FetchContent)
	FetchContent_Declare(ext_glfw SYSTEM GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG master)
	FetchContent_MakeAvailable(ext_glfw)
	
	get_target_property(glfw_INCL glfw INTERFACE_INCLUDE_DIRECTORIES)
	target_include_directories(glfw SYSTEM INTERFACE ${glfw_INCL})
endif()

option(VX_CXX_MODE OFF)
if (VX_CXX_MODE)
	enable_language(CXX)
	set(CMAKE_CXX_EXTENSIONS OFF)
	set(CMAKE_CXX_STANDARD 11)
	
	file(GLOB_RECURSE CFILES "${VX_SRC_DIR}/*.c" "${VX_SRC_DIR}/*.h")
	set_source_files_properties(${CFILES} PROPERTIES LANGUAGE CXX)

	set(VX_COMPILER ${CMAKE_CXX_COMPILER_ID})
	if (NOT MSVC)
		set(VX_COMPILER_CXX_SETTINGS "-Wno-deprecated -Wno-c++98-compat")
	endif()
else()
	enable_language(C)
	set(CMAKE_C_EXTENSIONS OFF)
	set(CMAKE_C_STANDARD 99)
	set(VX_COMPILER ${CMAKE_C_COMPILER_ID})

	if (NOT MSVC)
		set(VX_COMPILER_SETTINGS "-fno-asynchronous-unwind-tables")
	endif()
endif()

if (MSVC)
	set(VX_COMPILER_SETTINGS "/W4")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
else()
	if(VX_COMPILER MATCHES "Clang")
		set(VX_COMPILER_SETTINGS "${VX_COMPILER_SETTINGS} -Weverything -Wno-unsafe-buffer-usage -Wno-declaration-after-statement")
	else()
		set(VX_COMPILER_SETTINGS "${VX_COMPILER_SETTINGS} -Wall -Wextra -pedantic -Wshadow -Wundef -Wconversion")
	endif()
endif()

if (VX_CXX_MODE)
	set(CMAKE_CXX_FLAGS "${VX_COMPILER_SETTINGS} ${VX_COMPILER_CXX_SETTINGS} ${CMAKE_CXX_FLAGS}")
else()
	set(CMAKE_C_FLAGS "${VX_COMPILER_SETTINGS} ${CMAKE_C_FLAGS}")
endif()

add_executable(${PROJECT_NAME}
	${VX_SRC_DIR}/events/chat.c
	${VX_SRC_DIR}/events/commands_list.c
	${VX_SRC_DIR}/events/commands.c
	${VX_SRC_DIR}/events/events.c
	${VX_SRC_DIR}/events/keys_list.c
	${VX_SRC_DIR}/events/keys.c
	${VX_SRC_DIR}/events/mouse.c
	${VX_SRC_DIR}/events/screenshot.c
	${VX_SRC_DIR}/events/window.c

	${VX_SRC_DIR}/graphics/glctx.c
	${VX_SRC_DIR}/graphics/glfuncs.c

	${VX_SRC_DIR}/io/files.c
	${VX_SRC_DIR}/io/format.c
	${VX_SRC_DIR}/io/logs.c

	${VX_SRC_DIR}/player/camera.c
	${VX_SRC_DIR}/player/edit.c
	${VX_SRC_DIR}/player/frustum.c
	${VX_SRC_DIR}/player/inventory.c
	${VX_SRC_DIR}/player/movement.c
	${VX_SRC_DIR}/player/raycast.c

	${VX_SRC_DIR}/text/text_mgr.c
	${VX_SRC_DIR}/text/text_obj.c

	${VX_SRC_DIR}/shaders/loader.c
	${VX_SRC_DIR}/shaders/programs.c
	${VX_SRC_DIR}/shaders/textures.c
	${VX_SRC_DIR}/shaders/ubo.c

	${VX_SRC_DIR}/utils/dyarray.c
	${VX_SRC_DIR}/utils/noise.c
	${VX_SRC_DIR}/utils/png.c
	${VX_SRC_DIR}/utils/rand.c
	${VX_SRC_DIR}/utils/thread.c

	${VX_SRC_DIR}/values/elements.c
	${VX_SRC_DIR}/values/state.c

	${VX_SRC_DIR}/vector/mat4.c

	${VX_SRC_DIR}/world/generate.c
	${VX_SRC_DIR}/world/locate.c
	${VX_SRC_DIR}/world/map.c
	${VX_SRC_DIR}/world/mesh.c
	${VX_SRC_DIR}/world/region.c
	${VX_SRC_DIR}/world/sky.c
	${VX_SRC_DIR}/world/modify.c

	${VX_SRC_DIR}/main.c
)

if (MSVC)
	set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${VX_SRC_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

if (ZLIB_FOUND)
	target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
endif()

if (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE m pthread)
endif()
