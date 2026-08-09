add_executable(trivial_tests
	ecs/world_tests.cpp
	math/angle.cpp
	math/vector.cpp
	math/matrix.cpp
	math/transform.cpp
	memory/page_allocator.cpp
	task/task_system.cpp
)

FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # On Windows, prevent gtest from overriding the parent project's compiler/linker settings
FetchContent_MakeAvailable(googletest)

target_link_libraries(trivial_tests
	PRIVATE
		trivial
		GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(trivial_tests)
