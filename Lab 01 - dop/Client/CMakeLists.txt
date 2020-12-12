cmake_minimum_required(VERSION 3.17)
project(Client)

set(CMAKE_CXX_STANDARD 14)

add_executable(Client main.cpp client.cpp client.h AppInterface.h AppInterface_c.c)