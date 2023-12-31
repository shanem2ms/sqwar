cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

if (MSVC)
add_compile_options(/await:strict /Zc:__cplusplus)
endif ()

set (BGFX_INCLUDE_ROOT ${CMAKE_INSTALL_PREFIX}/include)
set (BGFX_INCLUDE ${BGFX_INCLUDE_ROOT}/bgfx)
set (BGFX_LIB ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MainBinaryDir})
include(/homep4/vcpkg/scripts/buildsystems/vcpkg.cmake)

################################################################################
# Target
################################################################################
set(Main_Files
"map.cpp"
"Resource.h"
"map.h"
"map.rc"
)

add_executable(${PROJECT_NAME} WIN32 ${Main_Files})

set (PLATFORMLIBS
"iphlpapi;"
"ws2_32;"
"d3dcompiler;"
"dxgi;"
"dxguid;"
"d3d11;"
"Shcore;")

find_package(ffmpeg REQUIRED)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC 
    ${FFMPEG_LIBRARIES}
    bgfx::bgfx
    bgfx::bimg
    bgfx::bx
    mdns
    core 
    ${PLATFORMLIBS}
       )
       
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/."
    "${CMAKE_BINARY_DIR}/."
    "${BGFX_INCLUDE_ROOT}"
    "${BX_INC_DIR_COMPAT}"
    "${BGFX_INC_DIR}"
    )

add_compile_definitions(BX_CONFIG_DEBUG=${BX_CONFIG_DEBUG})