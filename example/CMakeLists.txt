add_executable(omnibridge_hid_example)
target_sources(omnibridge_hid_example
PRIVATE
    main.cpp
)

target_link_libraries(omnibridge_hid_example
PRIVATE
    omnibridge_hid_cpp
)