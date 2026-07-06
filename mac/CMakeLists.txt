# YouTube Source for VirtualDJ - macOS build
#
# Requirements: macOS with Xcode command-line tools + CMake
#   brew install cmake
#
# Build (from this "mac" folder, ON A MAC):
#   cmake -B build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#
# Output: build/YouTubeSource.bundle
# Install to: ~/Library/Application Support/VirtualDJ/PluginsMacArm/OnlineSources/  (Apple Silicon)
#         or: ~/Library/Application Support/VirtualDJ/Plugins64/OnlineSources/     (Intel)

cmake_minimum_required(VERSION 3.20)
project(YouTubeSource LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(YouTubeSource MODULE
    ${SRC}/YouTubeSource.cpp
    ${SRC}/YtDlpRunner.cpp
    ${SRC}/CacheIndex.cpp
    ${SRC}/WebViewHostMac.mm
)

target_include_directories(YouTubeSource PRIVATE ${SRC})

set_target_properties(YouTubeSource PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "bundle"
    OUTPUT_NAME "YouTubeSource"
)

target_link_libraries(YouTubeSource
    "-framework Cocoa"
    "-framework WebKit"
)

target_compile_options(YouTubeSource PRIVATE -fobjc-arc)
