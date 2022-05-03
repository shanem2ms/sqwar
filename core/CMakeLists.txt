set (BGFX_INCLUDE_ROOT ${CMAKE_INSTALL_PREFIX}/include)
set (BGFX_INCLUDE ${BGFX_INCLUDE_ROOT}/bgfx)
set (BGFX_LIB ${CMAKE_INSTALL_PREFIX}/lib)

if (MSVC)
add_compile_options(/await:strict)
set(BX_INC_DIR_COMPAT ${BGFX_INCLUDE_ROOT}/compat/msvc)
elseif (APPLE)
set(BX_INC_DIR_COMPAT ${BGFX_INCLUDE_ROOT}/compat/osx)
endif()

add_subdirectory(shaders)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "StdIncludes.h"
    "Application.h"
    "Engine.h"
    "Hud.h"
    "SceneItem.h"
    "DepthPts.h"
    "FFmpeg.h"
    "PlanesVis.h"
    "FaceVis.h"
    "Network.h"
    "World.h"
    "Mesh.h"
    "PtsVis.h"
)  

source_group("Header Files" FILES ${Header_Files})

if (SAM_COROUTINE)
set(Coroutine_Files
    "cppcoro/lib/auto_reset_event.cpp"
    "cppcoro/lib/lightweight_manual_reset_event.cpp"
    "cppcoro/lib/spin_mutex.cpp"
    "cppcoro/lib/spin_wait.cpp"
    "cppcoro/lib/static_thread_pool.cpp"
    "cppcoro/lib/win32.cpp"
)
endif()

set(Source_Files
    "Application.cpp"
    "Alignment.cpp"
    "Engine.cpp"
    "Hud.cpp"
    "SceneItem.cpp"
    "DepthPts.cpp"
    "DepthPlanes.cpp"
    "PlanesVis.cpp"
    "FFmpeg.cpp"
    "FaceVis.cpp"
    "World.cpp"
    "Mesh.cpp"
    "Network.cpp"
    "PtsVis.cpp"
    "imgui/imgui.cpp"
    "imgui/dear-imgui/imgui.cpp"
    "imgui/dear-imgui/imgui_draw.cpp"
    "imgui/dear-imgui/imgui_widgets.cpp"
    "imgui/dear-imgui/imgui_tables.cpp"
    )

source_group("Source Files" FILES ${Source_Files} ${Main_Files})

set(ALL_FILES
    
    ${Header_Files} 
    ${Source_Files}
    ${Coroutine_Files}
    ${Main_Files}
)

add_library(core STATIC ${ALL_FILES})

target_precompile_headers(core PRIVATE "StdIncludes.h")

add_dependencies(core shaders)
################################################################################
# Include directories
################################################################################

message("BGFX_INC_DIR = ${BGFX_INC_DIR}")

target_include_directories(core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/."
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/."
    "${CMAKE_SOURCE_DIR}/mdns/."
    "${VCPKG_INSTALL_PATH}/include"
    "./asio/include"
    ${BGFX_INCLUDE_ROOT}
    ${BX_INC_DIR_COMPAT}
    )

add_compile_definitions(DLLX=;PRId64="I64d";BX_CONFIG_DEBUG=${BX_CONFIG_DEBUG})
