cmake_minimum_required (VERSION 3.10)
project (ttt_agent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include directories
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(ttt_agent ${MY_SOURCES})

# output directories (optional)
set_target_properties(ttt_agent PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "ttt_agent"
)