cmake_minimum_required(VERSION 3.28)
project(boids CXX)

include(FetchContent)
FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git SYSTEM EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(SFML)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    add_compile_options(-Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast)
elseif(MSVC)
    add_compile_options(/WX /W4 /permissive-)
endif()

add_executable(boids src/main.cpp src/Boid.cpp)
target_link_libraries(boids PRIVATE SFML::Graphics)
target_compile_features(boids PRIVATE cxx_std_20)
target_compile_definitions(boids PRIVATE FONT_PATH="${CMAKE_SOURCE_DIR}/data")

add_custom_target(format
    COMMAND clang-format -i `git ls-files *.hpp *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(tidy
    COMMAND run-clang-tidy -p ${CMAKE_BINARY_DIR} `git ls-files *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(run COMMAND boids)
