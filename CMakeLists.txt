cmake_minimum_required(VERSION 3.28.3)

project("OmniBridge HID c++" LANGUAGES C CXX )

add_subdirectory(example)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)