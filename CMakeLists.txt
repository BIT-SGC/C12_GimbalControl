cmake_minimum_required(VERSION 3.12)
project(gimbal_control VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(Threads REQUIRED)


add_library(gimbal_socket SHARED
    src/practical_socket/PracticalSocket.cc)

add_library(gimbal_loguru SHARED
    src/loguru/loguru.cc)

add_library(gimbal_control SHARED
    src/gimbal_ctrl.cc
)
    
include(GNUInstallDirs)

# 修改安装路径到 /usr/lib/
install(TARGETS gimbal_control gimbal_loguru gimbal_socket
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 仅安装 gimbal_ctrl.h，避免污染
install(FILES 
    src/gimbal_ctrl.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gimbal_drv
)

install(FILES 
    src/practical_socket/PracticalSocket.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gimbal_drv/practical_socket/
)

install(FILES 
    src/loguru/loguru.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gimbal_drv/loguru/)

add_executable(GimbalCtrl_test
    src/main.cc
)

target_link_libraries(gimbal_loguru
    PRIVATE
        pthread
        dl
)

target_link_libraries(gimbal_control
    PRIVATE
        Threads::Threads
        gimbal_socket
        gimbal_loguru
)

target_link_libraries(GimbalCtrl_test
    PRIVATE
        gimbal_control
)

# ========================
# CPack Debian Package 配置
# ========================

set(CPACK_GENERATOR "DEB")
# 维护者邮箱（纯文本！）
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Cao.wangrenbo <cao.wangrenbo@yandex.com>")
# 依赖管理
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)    # 自动检测依赖
# 包元数据
set(CPACK_DEBIAN_PACKAGE_NAME "gimbal-control")
set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
set(CPACK_PACKAGE_VENDOR "BIT-ZW")

include(CPack)

# 架构检测（保留原有逻辑）
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
endif()
