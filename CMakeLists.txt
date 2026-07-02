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

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

set(TRIVIAL_PRIVATE_SOURCES
	src/engine.cpp
	src/application.cpp
	src/core/log.cpp
	src/gpu/context.cpp
	src/platform/window.cpp
	src/platform/glfw/window.cpp
	src/render/renderer.cpp
	src/rhi/backend.cpp
	src/rhi/vulkan/backend.cpp
	src/rhi/vulkan/result.cpp
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

# TODO: Sort this mess out
#       Need to decide if want to keep logging and assertion into just source files or move to public since there is a
#       lot of templated stuff that would benefit
#       Library is going to be static so that would be fine, just ought to be careful as they are meant to be internal
#       macros
target_compile_definitions(trivial
	PUBLIC
		TRIVIAL_ENABLE_ASSERTS=$<BOOL:${TRIVIAL_ENABLE_ASSERTS}>
		TRIVIAL_ENABLE_SLOW_ASSERTS=$<BOOL:${TRIVIAL_ENABLE_SLOW_ASSERTS}>
		TRIVIAL_ENABLE_VULKAN_VALIDATION=$<BOOL:${TRIVIAL_ENABLE_VULKAN_VALIDATION}>

		TRIVIAL_CONFIG_DEBUG=$<CONFIG:Debug>
		TRIVIAL_CONFIG_RELEASE=$<CONFIG:Release>
		TRIVIAL_CONFIG_RELWITHDEBINFO=$<CONFIG:RelWithDebInfo>

		TRIVIAL_PLATFORM_GLFW

	PRIVATE
		TRIVIAL_ENABLE_LOGGING=$<BOOL:${TRIVIAL_ENABLE_LOGGING}>
)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_link_libraries(trivial
	PRIVATE
		Vulkan::Vulkan
		glfw
)

if(TRIVIAL_ENABLE_TRACY)
	find_package(Tracy CONFIG REQUIRED)

	target_link_libraries(trivial
		PUBLIC
			Tracy::TracyClient
	)

	target_compile_definitions(trivial
		PUBLIC
			TRACY_ENABLE
			TRIVIAL_ENABLE_TRACY=1
	)
else()
	target_compile_definitions(trivial
		PUBLIC
			TRIVIAL_ENABLE_TRACY=0
	)
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------

enable_testing()

add_subdirectory(tests)

# -----------------------------------------------------------------------------
# Compiler warnings for Trivial itself - not used be games using
# -----------------------------------------------------------------------------

if(MSVC)
	target_compile_options(trivial PRIVATE
		$<$<COMPILE_LANGUAGE:C>:/W4>
		$<$<COMPILE_LANGUAGE:CXX>:/W4>
		$<$<COMPILE_LANGUAGE:CXX>:/permissive->
		$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
	)
else()
	target_compile_options(trivial PRIVATE
		-Wall
		-Wextra
		-Wpedantic
		-Wshadow
	)
endif()
