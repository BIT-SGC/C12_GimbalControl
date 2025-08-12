cmake_minimum_required(VERSION 3.12)
project(GimbalCtrl LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(Threads REQUIRED)


add_library(PracticalSocket SHARED
    src/practical_socket/PracticalSocket.cc)

add_library(loguru SHARED
    src/loguru/loguru.cc)

add_executable(GimbalCtrl
    src/main.cc
    src/gimbal_ctrl.cc
)

target_link_libraries(GimbalCtrl
    PRIVATE
        Threads::Threads
        PracticalSocket
        loguru
)
