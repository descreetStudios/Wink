add_executable(Sandbox
    "Source/Main.cpp")

target_include_directories(Sandbox 
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source)

# --- Fetch Content --- #
# TRACY
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.13.1
)
FetchContent_MakeAvailable(tracy)

target_link_libraries(Sandbox
PRIVATE 
    Engine
    Tracy::TracyClient
)