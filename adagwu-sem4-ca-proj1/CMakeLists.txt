### _____________________________________________________________________________________
cmake_minimum_required(VERSION 3.20)
project(oglproj1)
set(CMAKE_CXX_STANDARD 20)
### _________________________________________________________________________________________________________


### Properties ###
### _____________________________________________________________________________________
set(OUTPUT_BASE_DIR ${CMAKE_BINARY_DIR})
# For single-configuration generators (e.g. Makefile, Ninja)
if(NOT CMAKE_CONFIGURATION_TYPES)
	set(OUTPUT_BASE_DIR ${OUTPUT_BASE_DIR}/bin)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BASE_DIR})
### _____________________________________________________________________________________
set(GLFW_BUILD_EXAMPLES   OFF CACHE BOOL "Build the GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS      OFF CACHE BOOL "Build the GLFW test programs" FORCE)
set(GLFW_BUILD_DOCS       OFF CACHE BOOL "Build the GLFW documentation" FORCE)
if(CMAKE_HOST_UNIX AND NOT APPLE)
	set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Use Wayland display server" FORCE)
	set(GLFW_BUILD_X11 ON CACHE BOOL "Use X11 display server" FORCE)
elseif(APPLE)
	# macOS: Use Cocoa (default), X11 only if XQuartz is available
	find_path(X11_INCLUDE_DIR X11/Xlib.h)
	if(X11_INCLUDE_DIR)
		message(STATUS "XQuartz detected, enabling X11 support in GLFW")
		set(GLFW_USE_X11 ON CACHE BOOL "Use X11 display server" FORCE)
	else()
		set(GLFW_USE_X11 OFF CACHE BOOL "Use X11 display server" FORCE)
	endif()    
elseif(WIN32)
	# No extra setup needed
endif()
### _________________________________________________________________________________________________________


### Add the subdirectories ###
### _____________________________________________________________________________________
add_subdirectory(src/glfw1)
### _____________________________________________________________________________________
add_subdirectory(src/glad1)
### _________________________________________________________________________________________________________


### Create the executable ###
### _____________________________________________________________________________________
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_executable(oglproj1  ${MY_SOURCES})
### _____________________________________________________________________________________


### Links to the executable ###
### _____________________________________________________________________________________
find_package(OpenGL REQUIRED)
target_link_libraries(oglproj1 PRIVATE glfw OpenGL::GL)
### _____________________________________________________________________________________
target_link_libraries(oglproj1 PRIVATE glad1)
### _________________________________________________________________________________________________________


### Set includde directories ###
### _____________________________________________________________________________________
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
### _____________________________________________________________________________________
target_include_directories(oglproj1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/glfw1/include)
### _____________________________________________________________________________________
target_include_directories(oglproj1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/glad1/include)
### _________________________________________________________________________________________________________