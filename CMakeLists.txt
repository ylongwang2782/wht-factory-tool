cmake_minimum_required(VERSION 3.16)

project(wht-factory-tool VERSION 1.0.0 LANGUAGES CXX)

execute_process(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 Qt 元对象系统、UI 文件和资源文件
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt 模块
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort Network)

# 添加protocol子目录
add_subdirectory(protocol)

# 添加可执行文件
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

# 链接 Qt 库和protocol库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Network
    WhtsProtocol
)

# 对于MSVC编译器，添加必要的编译器标志
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# # 设置可执行文件图标（Windows）
# if(WIN32)
#     set_target_properties(${PROJECT_NAME} PROPERTIES
#         WIN32_EXECUTABLE TRUE
#         ICON "${CMAKE_CURRENT_SOURCE_DIR}/wht-factory-tool.ico"
#     )
# endif()
