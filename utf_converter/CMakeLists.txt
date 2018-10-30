cmake_minimum_required (VERSION 3.12)
project (utf_string_conversion C)

add_library (utf_string_conversion utf_string_conversion.c)

add_executable (test test.c)
target_link_libraries (test utf_string_conversion)
