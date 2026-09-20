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
	memory/segment_allocator_multi_thread.cpp
	memory/segment_allocator_single_thread.cpp
	sync/condition_variable_multi_thread.cpp
	sync/condition_variable_single_thread.cpp
	sync/event_multi_thread.cpp
	sync/event_single_thread.cpp
	sync/latch_multi_thread.cpp
	sync/latch_single_thread.cpp
	sync/mutex_multi_thread.cpp
	sync/mutex_single_thread.cpp
	sync/semaphore_multi_thread.cpp
	sync/semaphore_single_thread.cpp
	sync/spin_lock_single_thread.cpp
	sync/spin_lock_multi_thread.cpp
	task/task_system.cpp
	thread/custom_thread_type.cpp
)

target_link_libraries(trivial_tests
	PRIVATE
		trivial
		GTest::gtest_main
)

target_include_directories(trivial PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories(trivial_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)

include(GoogleTest)

gtest_discover_tests(trivial_tests)
