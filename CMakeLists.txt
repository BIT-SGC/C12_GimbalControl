cmake_minimum_required(VERSION 3.12)
project(gimbal_control LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(Threads REQUIRED)


add_library(PracticalSocket SHARED
    src/practical_socket/PracticalSocket.cc)

add_library(loguru SHARED
    src/loguru/loguru.cc)

add_library(gimbal_control SHARED
    src/gimbal_ctrl.cc
)
    
add_executable(GimbalCtrl_test
    src/main.cc
)

target_link_libraries(gimbal_control
    PRIVATE
        Threads::Threads
        PracticalSocket
        loguru
)

target_link_libraries(GimbalCtrl_test
    PRIVATE
        gimbal_control
)

