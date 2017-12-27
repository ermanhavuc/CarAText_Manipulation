cmake_minimum_required(VERSION 3.8)
project(CarAText_Manipulation)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c)

add_executable(CarAText_Manipulation ${SOURCE_FILES})