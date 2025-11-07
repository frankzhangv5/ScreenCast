cmake_minimum_required(VERSION 3.12)
project(mirror_server)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set target architecture
if(DEFINED OHOS_TARGET_ARCH)
    set(CMAKE_CXX_COMPILER_TARGET ${OHOS_TARGET_ARCH})
    set(CMAKE_C_COMPILER_TARGET ${OHOS_TARGET_ARCH})
    else()
        message(FATAL_ERROR "Undefined OHOS_TARGET_ARCH")
endif()

# Add executable
add_executable(mirror_server
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/main.cpp
    src/OHOSDevice.cpp
)

# Include directories
target_include_directories(mirror_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${OHOS_SDK_HOME}/native/sysroot/usr/include
    ${OHOS_SDK_HOME}/native/sysroot/usr/include/ohos
    ${OHOS_SDK_HOME}/native/sysroot/usr/include/multimedia
    ${OHOS_SDK_HOME}/native/sysroot/usr/include/deviceinfo
    ${OHOS_SDK_HOME}/native/sysroot/usr/include/hiviewdfx
    ${OHOS_SDK_HOME}/native/sysroot/usr/include/window_manager
)

# Compile definitions
target_compile_definitions(mirror_server PRIVATE
    __OHOS__
    PORT=12345
)

# Compile options
target_compile_options(mirror_server PRIVATE
    -Wall
)

# Link directories
target_link_directories(mirror_server PRIVATE
    ${OHOS_SDK_HOME}/native/sysroot/usr/lib/${OHOS_TARGET_ARCH}
    ${OHOS_SDK_HOME}/native/sysroot/usr/lib/ohos
)

# Link libraries
target_link_libraries(mirror_server
    -lhilog_ndk.z
    -ldeviceinfo_ndk.z
    -lnative_avscreen_capture
    -lnative_display_manager
    -lnative_media_codecbase
    -lnative_media_core
    -lnative_media_venc
    -lpthread
)

# Install configuration
install(TARGETS mirror_server
    RUNTIME DESTINATION bin
)