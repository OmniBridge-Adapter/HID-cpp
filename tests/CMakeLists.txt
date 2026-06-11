cmake_minimum_required(VERSION 3.13)

include(FetchContent)

set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(omnibridge_hid_cpp_tests
    ${CMAKE_CURRENT_LIST_DIR}/test_proxy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_report_descriptors.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_report_primitives.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_report_data_access.cpp
)

target_link_libraries(omnibridge_hid_cpp_tests
    PRIVATE
        omnibridge_hid_cpp
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(omnibridge_hid_cpp_tests)
