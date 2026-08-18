add_executable(cycletest main.cpp)

target_link_libraries(cycletest PRIVATE
    gs2_mmu
    gs2_cpu 
    gs2_cpu_new
    gs2_debugger
    gs2_video_scanner
)

add_executable(cycletest816 main816.cpp)

target_link_libraries(cycletest816 PRIVATE
    gs2_mmu
    gs2_cpu 
    gs2_cpu_new
    gs2_debugger
    gs2_video_scanner
)