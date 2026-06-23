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

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)

# FASTGLTF
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG v0.9.0
)
FetchContent_MakeAvailable(fastgltf)

# MIKKTSPACE
FetchContent_Declare(
    mikktspace
    GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git
    GIT_TAG master
)
FetchContent_MakeAvailable(mikktspace)
add_library(MikkTSpace STATIC ${mikktspace_SOURCE_DIR}/mikktspace.c)
target_include_directories(MikkTSpace PRIVATE ${mikktspace_SOURCE_DIR})
target_include_directories(Engine PRIVATE ${mikktspace_SOURCE_DIR})

# GLAD
add_library(glad ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/glad/include)

# STBI
target_include_directories(Engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/stb/include)

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
    fastgltf
    MikkTSpace
)

target_link_libraries(Engine
PUBLIC
    eventpp::eventpp
    EnTT::EnTT
    glm::glm
    glad
)

target_sources(Engine 
PRIVATE
    # --- Core --- #
    "Source/Core/Main.cpp" 
    "Source/Core/Application.cpp" 
    "Source/Core/Logger.cpp" 
    "Source/Core/Time.cpp" 
    "Source/Core/FileWatcher.cpp" 
    "Source/Core/Input.cpp" 
    "Source/Core/Window.cpp" 

    # --- ECS --- #
    "Source/ECS/Entity.cpp" 
    "Source/ECS/Scene.cpp" 
   
    # --- Components --- #
    "Source/ECS/Components/TransformComponent.cpp" 

    # --- Systems --- #
    "Source/ECS/Systems/TransformSystem.cpp"
    "Source/ECS/Systems/ModelSystem.cpp" 
    
    # --- Graphics Abstractions --- #
    "Source/GFX/VAO.cpp" 
    "Source/GFX/Buffer.cpp" 
    "Source/GFX/Texture.cpp" 
    "Source/GFX/Framebuffer.cpp" 
    "Source/GFX/Shader.cpp" 
    
    # --- Graphics Resources --- #
    "Source/GFX/Resource/MeshPool.cpp" 
    "Source/GFX/Resource/ShaderPool.cpp" 
    "Source/GFX/Resource/TexturePool.cpp" 
    "Source/GFX/Resource/MaterialPool.cpp" 
    "Source/GFX/Resource/ModelPool.cpp" 
    
    # --- Content --- #
    "Source/Content/Image.cpp" 
    "Source/Content/GLTFModel.cpp" 

    # --- Graphics --- #
    "Source/GFX/Material.cpp" 
    "Source/GFX/Camera.cpp" 
    "Source/GFX/Renderer.cpp")

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