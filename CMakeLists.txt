cmake_minimum_required(VERSION 4.2)
project(TheClient)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(client main.cpp)

# 链接 Winsock2 库
target_link_libraries(client PRIVATE ws2_32)

# 如果使用 MSVC，禁用 SDL 检查以避免一些安全函数警告（可选）
if(MSVC)
    target_compile_options(winsock_client PRIVATE /W3 /EHsc)
    target_compile_options(winsock_client PRIVATE
            $<$<CONFIG:Debug>:/MTd>
            $<$<NOT:$<CONFIG:Debug>>:/MT>)
    set_target_properties(winsock_client PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
