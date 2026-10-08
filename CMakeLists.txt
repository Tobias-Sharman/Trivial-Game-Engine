cmake_minimum_required(VERSION 3.30)

set(CMAKE_OSX_DEPLOYMENT_TARGET 14.4)
# Linux minimum not set here, based around buld environment
# Windows minimum stuff can be seen in the platform definitions of this file

project(
	Trivial
	VERSION 0.1.0
	# LANGUAGES C CXX
	LANGUAGES CXX
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
option(TRIVIAL_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(TRIVIAL_EXPORT_DEPENDENCY_COMPILE_COMMANDS "Include third-party targets in compile_commands.json" OFF)
option(TRIVIAL_WARNINGS_AS_ERRORS "Treat compiler warnings in the engine as errors" OFF)

set(TRIVIAL_MEMORY_DEBUG "AUTO" CACHE STRING "Allocation level memory debugging (AUTO/ON/OFF)")
set_property(CACHE TRIVIAL_MEMORY_DEBUG PROPERTY STRINGS AUTO ON OFF)

set(TRIVIAL_MEMORY_DEBUG_STATS "AUTO" CACHE STRING "Segment level memory statistics (AUTO/ON/OFF)")
set_property(CACHE TRIVIAL_MEMORY_DEBUG_STATS PROPERTY STRINGS AUTO ON OFF)

set(TRIVIAL_MEMORY_DECOMMIT "AUTO" CACHE STRING "Return freed memory to the OS (AUTO/ON/OFF)")
set_property(CACHE TRIVIAL_MEMORY_DECOMMIT PROPERTY STRINGS AUTO ON OFF)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

set(TRIVIAL_PRIVATE_SOURCES
	src/engine.cpp
	src/application.cpp
	src/layers.cpp
	src/core/assert.cpp
	src/core/log.cpp
	src/core/math/mat4.cpp
	src/core/memory/memory_capabilities.cpp
	src/core/memory/segment_allocator.cpp
	src/core/sync/condition_variable.cpp
	src/core/sync/escalating_lock.cpp
	src/core/sync/event.cpp
	src/core/sync/latch.cpp
	src/core/sync/mutex.cpp
	src/core/sync/parker.cpp
	src/core/sync/semaphore.cpp
	src/core/thread/thread.cpp
	src/core/thread/thread_stack_allocator.cpp
	src/core/time/time.cpp
	src/ecs/component_store.cpp
	src/core/time/time_capabilities.cpp
	src/gpu/context.cpp
	src/platform/window.cpp
	src/platform/glfw/window.cpp
	src/render/renderer.cpp
	src/render/shapes.cpp
	src/rhi/backend.cpp
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
	# PRIVATE
	# 	c_std_99
)

if(TRIVIAL_ENABLE_TSAN AND NOT MSVC)
	target_compile_options(trivial PUBLIC -fsanitize=thread)
	target_link_options(trivial PUBLIC -fsanitize=thread)
endif()

target_compile_definitions(trivial PRIVATE TRIVIAL_SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/")

target_compile_definitions(trivial
	PRIVATE
		TRIVIAL_ENGINE_NAME="${PROJECT_NAME}"
		TRIVIAL_ENGINE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
		TRIVIAL_ENGINE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
		TRIVIAL_ENGINE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# -----------------------------------------------------------------------------
# Platform definitions
# -----------------------------------------------------------------------------

if(WIN32)
	target_compile_definitions(trivial PUBLIC _WIN32_WINNT=0x0A00 NTDDI_VERSION=0x0A000005)

	target_link_libraries(trivial PRIVATE Synchronization onecore)
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

include(FetchContent)

find_package(Vulkan REQUIRED)

if(NOT TRIVIAL_EXPORT_DEPENDENCY_COMPILE_COMMANDS)
	set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
endif()

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
set(TRACY_ENABLE ${TRIVIAL_ENABLE_TRACY} CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tracy)

unset(CMAKE_EXPORT_COMPILE_COMMANDS)

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

set(TRIVIAL_MEMORY_DEBUG_DEFINE "")
if(NOT TRIVIAL_MEMORY_DEBUG STREQUAL "AUTO")
	set(TRIVIAL_MEMORY_DEBUG_DEFINE "#define TRIVIAL_ENABLE_MEMORY_DEBUG $<BOOL:${TRIVIAL_MEMORY_DEBUG}>")
endif()

set(TRIVIAL_MEMORY_DEBUG_STATS_DEFINE "")
if(NOT TRIVIAL_MEMORY_DEBUG_STATS STREQUAL "AUTO")
	set(TRIVIAL_MEMORY_DEBUG_STATS_DEFINE "#define TRIVIAL_ENABLE_MEMORY_DEBUG_STATS $<BOOL:${TRIVIAL_MEMORY_DEBUG_STATS}>")
endif()

set(TRIVIAL_MEMORY_DECOMMIT_DEFINE "")
if(NOT TRIVIAL_MEMORY_DECOMMIT STREQUAL "AUTO")
	set(TRIVIAL_MEMORY_DECOMMIT_DEFINE "#define TRIVIAL_MEMORY_ENABLE_DECOMMIT $<BOOL:${TRIVIAL_MEMORY_DECOMMIT}>")
endif()

configure_file(include/trivial/core/build_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/build_config.h.in @ONLY)

file(GENERATE
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>/trivial/core/build_config.h
	INPUT ${CMAKE_CURRENT_BINARY_DIR}/build_config.h.in
)

target_include_directories(trivial
	PUBLIC
		${CMAKE_CURRENT_BINARY_DIR}/generated/$<CONFIG>
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
# Compiler options for Trivial itself - not to be used by games using
# -----------------------------------------------------------------------------

if(MSVC)
	target_compile_options(trivial PRIVATE
		/permissive-
		/Zc:__cplusplus
		/Zc:preprocessor
		/utf-8
		/EHs-c-

		/Gy
		/Gw

		/W4
		/external:env:INCLUDE
		/external:W0
		/wd4324
		/w44365
		/w44242
		/w44254
		/w44826
		/w44265
		/w44263
		/w44264
		/w45267
		/w45262
		/w44061
		/w44062
		/we4668
		/w45038
		$<$<BOOL:${TRIVIAL_WARNINGS_AS_ERRORS}>:/WX>
	)

	target_compile_definitions(trivial PRIVATE _HAS_EXCEPTIONS=0)

	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
	target_compile_options(trivial PRIVATE
		-fno-exceptions

		-ffunction-sections
		-fdata-sections
		$<$<NOT:$<CONFIG:Release>>:-fno-omit-frame-pointer>
		$<$<NOT:$<CONFIG:Release>>:-mno-omit-leaf-frame-pointer>
		$<$<CONFIG:Debug>:-ftrivial-auto-var-init=pattern>

		-Wall
		-Wextra
		-Wpedantic
		-Wshadow
		-Werror=undef
		-Wconversion
		-Wsign-conversion
		-Wdouble-promotion
		-Wold-style-cast
		-Wcast-qual
		-Wzero-as-null-pointer-constant
		-Wnon-virtual-dtor
		-Woverloaded-virtual
		-Wsuggest-override
		-Wextra-semi
		-Wctad-maybe-unsupported
		-Wdeprecated
		-Wimplicit-fallthrough
		-Wswitch-enum
		-Wnull-dereference
		-Wformat=2
		$<$<BOOL:${TRIVIAL_WARNINGS_AS_ERRORS}>:-Werror>
	)

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options(trivial PRIVATE
			-Wshadow-all
			-Wcast-align
			-Wcast-function-type-mismatch
			-Wweak-vtables
			-Wcovered-switch-default
			-Wunreachable-code-aggressive
			-Wcomma
			-Wmissing-prototypes
			-Wmissing-variable-declarations
			-Wnewline-eof
			-Wreserved-identifier
			-Werror=assume
		)
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(trivial PRIVATE
			-Wcast-align=strict
			-Wcast-function-type
			-Wuseless-cast
			-Wduplicated-cond
			-Wduplicated-branches
			-Wlogical-op
			-Wmissing-declarations
		)
	endif()
endif()
