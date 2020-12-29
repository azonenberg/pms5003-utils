cmake_minimum_required(VERSION 3.3)
project(pms5003-utils)

add_subdirectory("${PROJECT_SOURCE_DIR}/xptools")
add_subdirectory("${PROJECT_SOURCE_DIR}/log")
set_property(TARGET log PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET xptools PROPERTY POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS "-g -fopenmp ${WARNINGS} --std=c++11 -mtune=native -ffast-math")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fsanitize=address")

add_executable(pms5003 main.cpp)

target_link_libraries(
	pms5003
	xptools
	log)
