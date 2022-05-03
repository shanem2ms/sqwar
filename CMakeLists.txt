cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)
include(/homep4/vcpkg/scripts/buildsystems/vcpkg.cmake)
include (ExternalProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release") 
project(sqwar C CXX)

set(PROJECT_NAME sqwar)

set(BGFX_BUILD_EXAMPLES off)
set(BGFX_CUSTOM_TARGETS off)

if (APPLE)
    set(BGFX_BUILD_TOOLS off)
endif()

set (BX_CONFIG_DEBUG 0)

set (BGFX_INCLUDE_ROOT ${CMAKE_INSTALL_PREFIX}/include)
set (BGFX_INCLUDE ${BGFX_INCLUDE_ROOT}/bgfx)
set (BGFX_LIB ${CMAKE_INSTALL_PREFIX}/lib)

message("install path ${CMAKE_INSTALL_PREFIX}")

find_package(bgfx CONFIG REQUIRED )

set(MainBinaryDir ${CMAKE_BINARY_DIR})
set(BUNDLE_ID "com.mapbg.shane")

add_subdirectory(mdns)
add_subdirectory(core)
add_dependencies(core bgfx::bgfx)
add_dependencies(shaders bgfx::bgfx)

if (APPLE)
add_subdirectory(ios)
else()
add_subdirectory(win)
endif()
add_dependencies(sqwar core)
add_dependencies(sqwar mdns)

set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${BUNDLE_ID})
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "73CP3TPHE9")