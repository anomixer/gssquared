add_executable(montest main.cpp)

target_link_libraries(montest PRIVATE
    gs2_mmu
    gs2_debugger
    gs2_util
)