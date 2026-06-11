
add_library(omnibridge_hid_cpp INTERFACE)

target_sources(omnibridge_hid_cpp 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/hid/parser.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hid/proxy.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/hid/hid.cpp
)

target_include_directories(omnibridge_hid_cpp 
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(omnibridge_hid_cpp 
    INTERFACE 
        cxx_std_20
)
