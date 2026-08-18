add_executable(mmutest main.cpp)

target_link_libraries(mmutest PRIVATE
    gs2_mmu
)