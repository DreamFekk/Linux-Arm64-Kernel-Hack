cmake_minimum_required(VERSION 3.21.0)

set(NDK_PATH E:/ndk/android-ndk-r29)
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SYSTEM_VERSION 30)
set(ANDROID_PLATFORM 30)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_NDK ${NDK_PATH})
set(CMAKE_TOOLCHAIN_FILE ${NDK_PATH}/build/cmake/android.toolchain.cmake)

project(KernelDriver)
set(CMAKE_CXX_STANDARD 20)  # 改为 C++20

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/outputs/${ANDROID_ABI})

# 编译选项
add_compile_options(
        "$<$<COMPILE_LANGUAGE:C>:-std=c17>"
        "$<$<COMPILE_LANGUAGE:CXX>:-std=c++20>"  # 改为 C++20
        -fvisibility=hidden
        -fexceptions
        -w  # 禁用警告，与 mk 文件一致
        -O0

)

# 链接选项
add_link_options(

        -O0
        -flto
)

find_package(Vulkan REQUIRED)
add_definitions(
        -DVK_USE_PLATFORM_ANDROID_KHR
        -DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
        -DIMGUI_DISABLE_DEBUG_TOOLS  # 禁用imgui调试工具
        -DIMGUI_ENABLE_FREETYPE      # 启用imgui的freetype支持
)

# 头文件包含

# 源文件
file(GLOB SOURCES
        main.cpp
)

# 可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 预编译静态库配置
# FreeType 库


# 链接库
target_link_libraries(${PROJECT_NAME}

        log
        android

)

# 设置目标属性
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Stripping all symbols from ${PROJECT_NAME}"
)
