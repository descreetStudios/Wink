project(Engine LANGUAGES C CXX)

add_library(Engine)

# --- Fetch Content --- #
include(FetchContent)

# SPDLOG
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "Use external fmt library" FORCE)
FetchContent_MakeAvailable(spdlog)

# EVENTPP
include(FetchContent)
FetchContent_Declare(
    eventpp
    GIT_REPOSITORY https://github.com/wqking/eventpp
    GIT_TAG v0.1.3
)
FetchContent_MakeAvailable(eventpp)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# ENTT
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.16.0
)
set(ENTT_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENTT_USE_LIBCPP OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(entt)

# --- PCH --- #
target_precompile_headers(Engine 
PUBLIC 
    Include/WinkEngine/pch.hpp)

# --- Defines --- #
target_compile_definitions(Engine 
PUBLIC
    $<$<CONFIG:Debug>:WINK_DEBUG=1>
)

# --- Targets --- #
target_include_directories(Engine 
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include)
target_include_directories(Engine 
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_link_libraries(Engine
PRIVATE
    spdlog::spdlog
    glfw
)

target_link_libraries(Engine
PUBLIC
    eventpp::eventpp
    EnTT::EnTT
)

target_sources(Engine 
PRIVATE
    # --- Core --- #
    "Source/Core/Main.cpp" 
    "Source/Core/Application.cpp" 
    "Source/Core/Logger.cpp" 
    "Source/Core/Time.cpp" 
    "Source/Core/Window.cpp" "Source/ECS/Entity.cpp" "Source/ECS/Scene.cpp")

set(WINK_ENGINE_SHADERS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/GFX/Shaders"
    CACHE INTERNAL "Path to WinkEngine's default shader sources"
)

function(wink_copy_engine_shaders target)
    file(GLOB_RECURSE shader_files "${WINK_ENGINE_SHADERS_DIR}/*")

    add_custom_target(${target}_CopyShaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WINK_ENGINE_SHADERS_DIR}"
            "$<TARGET_FILE_DIR:${target}>/Shaders"
        DEPENDS ${shader_files}
        COMMENT "Copying engine shaders for ${target}"
    )

    add_dependencies(${target} ${target}_CopyShaders)
endfunction()