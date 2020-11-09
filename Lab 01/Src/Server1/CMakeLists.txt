cmake_minimum_required(VERSION 3.17)
project(Server1)

set(CMAKE_CXX_STANDARD 14)

add_executable(Server1 main.cpp server.cpp server.h)