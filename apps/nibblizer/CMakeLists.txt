add_executable(nibblizer main.cpp)

target_link_libraries(nibblizer PRIVATE
    gs2_devices_diskii_fmt
    gs2_util
)
