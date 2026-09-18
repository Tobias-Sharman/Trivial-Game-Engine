add_executable(trivial_sandbox)

set_target_properties(trivial_sandbox PROPERTIES
	CXX_EXTENSIONS OFF
)

target_sources(trivial_sandbox
	PRIVATE
		src/main.cpp
		src/boids/boid_rules.cpp
		src/boids/boids_simulation.cpp
)

target_compile_features(trivial_sandbox
	PRIVATE
		cxx_std_23
)

target_link_libraries(trivial_sandbox
	PRIVATE
		trivial
		glfw
)
