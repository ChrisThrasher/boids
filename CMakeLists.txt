cmake_minimum_required(VERSION 3.16)
project(boids CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 2.5.1 REQUIRED COMPONENTS graphics)

if(CMAKE_CXX_COMPILER_ID MATCHES "(GNU|Clang)")
    add_compile_options(-Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()

add_executable(boids src/main.cpp src/boid.cpp)
target_link_libraries(boids PRIVATE sfml-graphics)
target_compile_definitions(boids PRIVATE FONT_PATH="${CMAKE_SOURCE_DIR}/data")
target_compile_features(boids PRIVATE cxx_std_17)

add_custom_target(format
    COMMAND clang-format -i `git ls-files *.cpp *.h`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(tidy
    COMMAND clang-tidy -p ${CMAKE_BINARY_DIR} `git ls-files *.cpp`
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_custom_target(run COMMAND boids)
