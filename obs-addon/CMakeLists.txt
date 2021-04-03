cmake_minimum_required(VERSION 3.10)

if( NOT OBS_ROOT_DIR )
	set( OBS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../obs" CACHE STRING "Location of OBS" )
endif()

# Name of the project (will be the name of the plugin)
project(obs-addon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Build a shared library named after the project
file(GLOB SOURCE_FILES "*.cpp" "*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC} ${OBS_ROOT_DIR})

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_directories( ${PROJECT_NAME} PRIVATE "${OBS_ROOT_DIR}/build/libobs/Debug/" )
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} obs)
