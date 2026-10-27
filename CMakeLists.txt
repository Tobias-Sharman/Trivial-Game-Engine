cmake_minimum_required(VERSION 3.30)

project(
	TrivialWorkspace
	LANGUAGES CXX
)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(TRIVIAL_USER_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/tests/config/trivial_user_config.h)

add_subdirectory(engine)

enable_testing()

add_subdirectory(tests)

add_subdirectory(sandbox)
