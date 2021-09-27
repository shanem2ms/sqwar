cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)
set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)
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

set (BGFX_INCLUDE_ROOT ${CMAKE_INSTALL_PREFIX}/include)
set (BGFX_INCLUDE ${BGFX_INCLUDE_ROOT}/bgfx)
set (BGFX_LIB ${CMAKE_INSTALL_PREFIX}/lib)



if (MSVC)

ExternalProject_Add(
  Bgfx
  PREFIX "bgfx"
  PATCH_COMMAND ""
  UPDATE_COMMAND ""
  GIT_REPOSITORY "https://github.com/bkaradzic/bgfx.cmake.git"
  CMAKE_ARGS 
   -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DBGFX_BUILD_EXAMPLES=off -DBGFX_CUSTOM_TARGETS=off -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
   BUILD_BYPRODUCTS (
    ${BGFX_LIB}/bx${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${BGFX_LIB}/bimg${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${BGFX_LIB}/astc-codec${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${BGFX_LIB}/bgfx${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
)
elseif (APPLE)

ExternalProject_Add(
  Bgfx
  PREFIX "bgfx"
  PATCH_COMMAND ""
  UPDATE_COMMAND ""
  GIT_REPOSITORY "https://github.com/bkaradzic/bgfx.cmake.git"
  CMAKE_ARGS 
   -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DENABLE_ARC=FALSE -DBGFX_BUILD_EXAMPLES=off -DBGFX_CUSTOM_TARGETS=off -DBGFX_BUILD_TOOLS=off -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/ios.toolchain.cmake -DPLATFORM=OS64
   BUILD_BYPRODUCTS (
    ${BGFX_LIB}/bx${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${BGFX_LIB}/bimg${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${BGFX_LIB}/astc-codec${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${BGFX_LIB}/bgfx${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
)
endif()

set(MainBinaryDir ${CMAKE_BINARY_DIR})
set(BUNDLE_ID "com.mapbg.shane")

add_subdirectory(core)
add_dependencies(core Bgfx)
add_dependencies(shaders Bgfx)

if (APPLE)
add_subdirectory(ios)
add_dependencies(sqwar core)
else()
add_subdirectory(win)
add_dependencies(sqwar core)
endif()

set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${BUNDLE_ID})
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "73CP3TPHE9")