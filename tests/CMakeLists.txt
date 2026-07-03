find_package(GTest CONFIG REQUIRED)

add_executable(trivial_tests
	ecs/world_tests.cpp
	math/matrix.cpp
)

target_link_libraries(trivial_tests
	PRIVATE
		trivial
		GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(trivial_tests)
