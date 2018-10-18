cmake_minimum_required (VERSION 3.12)
project (utf_string_converter)

set(CMAKE_CXX_STANDARD 17)

add_library (utf_string_converter utf_string_converter.cpp)

add_executable (test test.cpp)
target_link_libraries (test utf_string_converter)
