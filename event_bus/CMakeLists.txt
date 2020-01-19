cmake_minimum_required(VERSION 3.14)

project (event_bus)
add_executable(event_bus event_bus.cpp event_bus.hpp tests.cpp ./external/catch2/catch.hpp)
set_property(TARGET event_bus PROPERTY CXX_STANDARD 17)
set_property(TARGET event_bus PROPERTY CXX_STANDARD_REQUIRED ON)