include(FetchContent)

FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.18.0
	GIT_SHALLOW TRUE
	SYSTEM
	EXCLUDE_FROM_ALL
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

target_compile_features(gtest PUBLIC cxx_std_23)
target_compile_features(gtest_main PUBLIC cxx_std_23)

add_executable(trivial_tests
	ecs/world_tests.cpp
	math/angle.cpp
	math/vector.cpp
	math/matrix.cpp
	math/transform.cpp
	memory/segment_allocator_single_thread.cpp
	memory/segment_allocator_multi_thread.cpp
	task/task_system.cpp
)

target_link_libraries(trivial_tests
	PRIVATE
		trivial
		GTest::gtest_main
)

target_include_directories(trivial PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories(trivial_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)

include(GoogleTest)

gtest_discover_tests(trivial_tests)
