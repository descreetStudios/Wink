add_executable(Sandbox
    "Source/Main.cpp")

target_include_directories(Sandbox 
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_link_libraries(Sandbox
PRIVATE 
    Engine
)

wink_copy_engine_shaders(Sandbox)