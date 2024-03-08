cmake_minimum_required(VERSION 3.8)
project(udp-emulator)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(list STATIC list_adders.c list_movers.c list_removers.c)
target_include_directories(list PUBLIC "${CMAKE_SOURCE_DIR}")

# For each application, do something similar:
# Assuming app source files are named app1.c, app2.c, etc.
# Repeat this block for each application, 
# changing the target name and source file as necessary
add_executable(partA-endpoint partA-endpoint.c)
add_executable(partA-middleend partA-middleend.c)
target_link_libraries(partA-middleend list)
target_include_directories(partA-middleend PUBLIC "${CMAKE_SOURCE_DIR}")


