cmake_minimum_required(VERSION 3.30)

project(
	Trivial
	VERSION 0.1.0
	LANGUAGES C CXX
)

# -----------------------------------------------------------------------------
# Engine options
# -----------------------------------------------------------------------------

option(TRIVIAL_ENABLE_ASSERTS "Enable fatal engine assertions" ON)
option(TRIVIAL_ENABLE_SLOW_ASSERTS "Enable expensive engine assertions" OFF)
option(TRIVIAL_ENABLE_LOGGING "Enable engine logging" ON)
option(TRIVIAL_ENABLE_VULKAN_VALIDATION "Enable Vulkan validation layers" ON)
option(TRIVIAL_ENABLE_TRACY "Enable Tracy profiling" OFF)
option(TRIVIAL_ENABLE_SIMD "Enable SIMD code paths engine wide" ON)
option(TRIVIAL_MATH_DISABLE_SIMD_BACKEND "Disable explicit SIMD math backends" OFF)
option(TRIVIAL_MATH_DISABLE_FMA "Disable explicit FMA in math backends" OFF)

set(TRIVIAL_MEMORY_DEBUG "AUTO" CACHE STRING "Allocation level memory debugging (AUTO/ON/OFF)")
set_property(CACHE TRIVIAL_MEMORY_DEBUG PROPERTY STRINGS AUTO ON OFF)

set(TRIVIAL_MEMORY_DEBUG_STATS "AUTO" CACHE STRING "Segment level memory statistics (AUTO/ON/OFF)")
set_property(CACHE TRIVIAL_MEMORY_DEBUG_STATS PROPERTY STRINGS AUTO ON OFF)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

set(TRIVIAL_PRIVATE_SOURCES
	src/engine.cpp
	src/application.cpp
	src/core/assert.cpp
	src/core/log.cpp
	src/core/math/mat4.cpp
	src/core/memory/segment_allocator.cpp
	src/gpu/context.cpp
	src/platform/window.cpp
	src/platform/glfw/window.cpp
	src/render/renderer.cpp
	src/render/shapes.cpp
	src/rhi/vulkan/allocator.cpp
	src/rhi/vulkan/backend.cpp
	src/rhi/vulkan/command.cpp
	src/rhi/vulkan/debug_messenger.cpp
	src/rhi/vulkan/device.cpp
	src/rhi/vulkan/instance.cpp
	src/rhi/vulkan/mesh.cpp
	src/rhi/vulkan/pipeline.cpp
	src/rhi/vulkan/physical_device.cpp
	src/rhi/vulkan/swapchain.cpp
	src/rhi/vulkan/sync.cpp
	src/task/task_graph.cpp
	src/task/task_system.cpp
	src/world/world.cpp
)

# -----------------------------------------------------------------------------
# Library
# -----------------------------------------------------------------------------

add_library(trivial STATIC)

set_target_properties(trivial PROPERTIES
	CXX_EXTENSIONS OFF
	C_EXTENSIONS OFF
)

target_sources(trivial
	PRIVATE
		${TRIVIAL_PRIVATE_SOURCES}
)

target_include_directories(trivial
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(trivial
	PUBLIC
		cxx_std_23
	PRIVATE
		c_std_99
)

target_compile_definitions(trivial
	PUBLIC
		TRIVIAL_ENABLE_ASSERTS=$<BOOL:${TRIVIAL_ENABLE_ASSERTS}>
		TRIVIAL_ENABLE_SLOW_ASSERTS=$<BOOL:${TRIVIAL_ENABLE_SLOW_ASSERTS}>
		TRIVIAL_ENABLE_LOGGING=$<BOOL:${TRIVIAL_ENABLE_LOGGING}>
		TRIVIAL_ENABLE_VULKAN_VALIDATION=$<BOOL:${TRIVIAL_ENABLE_VULKAN_VALIDATION}>
		TRIVIAL_ENABLE_SIMD=$<BOOL:${TRIVIAL_ENABLE_SIMD}>
		TRIVIAL_MATH_DISABLE_SIMD_BACKEND=$<BOOL:${TRIVIAL_MATH_DISABLE_SIMD_BACKEND}>
		TRIVIAL_MATH_DISABLE_FMA=$<BOOL:${TRIVIAL_MATH_DISABLE_FMA}>
		TRIVIAL_ENABLE_TRACY=$<BOOL:${TRIVIAL_ENABLE_TRACY}>
		$<$<BOOL:${TRIVIAL_ENABLE_TRACY}>:TRACY_ENABLE>

		TRIVIAL_PLATFORM_GLFW
)

target_compile_definitions(trivial PRIVATE TRIVIAL_SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/")

if(NOT TRIVIAL_MEMORY_DEBUG STREQUAL "AUTO")
	target_compile_definitions(trivial PUBLIC TRIVIAL_ENABLE_MEMORY_DEBUG=$<BOOL:${TRIVIAL_MEMORY_DEBUG}>)
endif()

if(NOT TRIVIAL_MEMORY_DEBUG_STATS STREQUAL "AUTO")
	target_compile_definitions(trivial PUBLIC TRIVIAL_ENABLE_MEMORY_DEBUG_STATS=$<BOOL:${TRIVIAL_MEMORY_DEBUG_STATS}>)
endif()

# -----------------------------------------------------------------------------
# Platform definitions
# -----------------------------------------------------------------------------

if(WIN32)
	target_compile_definitions(trivial
		PUBLIC
			_WIN32_WINNT=0x0A00
			WIN32_LEAN_AND_MEAN
			NOMINMAX
	)
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

include(FetchContent)

find_package(Vulkan REQUIRED)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG 3.5.1
	GIT_SHALLOW TRUE
	SYSTEM
	EXCLUDE_FROM_ALL
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
	vma
	GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
	GIT_TAG v3.4.0
	GIT_SHALLOW TRUE
	SYSTEM
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(vma)

FetchContent_Declare(
	tracy
	GIT_REPOSITORY https://github.com/wolfpld/tracy.git
	GIT_TAG v0.13.0
	GIT_SHALLOW TRUE
	SYSTEM
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(tracy)

if(NOT MSVC)
	target_compile_options(TracyClient PRIVATE -Wno-deprecated-declarations)
endif()

target_link_libraries(trivial
	PUBLIC
		Vulkan::Vulkan
		glfw
		Tracy::TracyClient
	PRIVATE
		GPUOpen::VulkanMemoryAllocator
)

# -----------------------------------------------------------------------------
# Build configuration
# -----------------------------------------------------------------------------

target_compile_definitions(trivial
	PUBLIC
		TRIVIAL_CONFIG_DEBUG=$<CONFIG:Debug>
		TRIVIAL_CONFIG_RELEASE=$<CONFIG:Release>
		TRIVIAL_CONFIG_RELWITHDEBINFO=$<CONFIG:RelWithDebInfo>
)

# -----------------------------------------------------------------------------
# Slang shader compilation
# -----------------------------------------------------------------------------

find_program(SLANGC_EXECUTABLE slangc REQUIRED)

function(compile_slang_shader TARGET SOURCE ENTRY STAGE OUTPUT)
	get_filename_component(OUTPUT_DIR "${OUTPUT}" DIRECTORY)
	file(MAKE_DIRECTORY "${OUTPUT_DIR}")

	add_custom_command(
		OUTPUT ${OUTPUT}
		COMMAND ${SLANGC_EXECUTABLE} -matrix-layout-row-major ${SOURCE} -target
		spirv -entry ${ENTRY} -stage ${STAGE} -o ${OUTPUT}
		DEPENDS ${SOURCE}
		COMMENT "Compiling ${SOURCE} (${STAGE}:${ENTRY}) to SPIR-V"
		VERBATIM
	)

	target_sources(${TARGET} PRIVATE ${OUTPUT})
endfunction()
compile_slang_shader(trivial
	${CMAKE_SOURCE_DIR}/shaders/flat_colour.slang
	vertexMain
	vertex
	${CMAKE_BINARY_DIR}/shaders/flat_colour.vert.spv
)
compile_slang_shader(trivial
	${CMAKE_SOURCE_DIR}/shaders/flat_colour.slang
	fragmentMain
	fragment
	${CMAKE_BINARY_DIR}/shaders/flat_colour.frag.spv
)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

enable_testing()

add_subdirectory(tests)

# -----------------------------------------------------------------------------
# Sandbox
# -----------------------------------------------------------------------------

add_subdirectory(sandbox)

# -----------------------------------------------------------------------------
# Compiler warnings for Trivial itself - not to be used by games using
# -----------------------------------------------------------------------------

if(MSVC)
	target_compile_options(trivial PRIVATE
		$<$<COMPILE_LANGUAGE:C>:/W4>
		$<$<COMPILE_LANGUAGE:CXX>:/W4>
		$<$<COMPILE_LANGUAGE:CXX>:/permissive->
		$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
		$<$<COMPILE_LANGUAGE:CXX>:/EHs-c->
	)
else()
	target_compile_options(trivial PRIVATE
		-Wall
		-Wextra
		-Wpedantic
		-Wshadow
		$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
	)
endif()
