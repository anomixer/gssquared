add_executable(denibblizer main.cpp)

target_link_libraries(denibblizer PRIVATE
    gs2_devices_diskii_fmt
    gs2_util
)
