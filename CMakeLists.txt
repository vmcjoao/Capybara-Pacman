cmake_minimum_required(VERSION 3.28)

project(CapybaraPacman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window Audio System)

add_executable(pacman pacman.cpp)
target_link_libraries(pacman PRIVATE SFML::Graphics SFML::Window SFML::Audio SFML::System)
