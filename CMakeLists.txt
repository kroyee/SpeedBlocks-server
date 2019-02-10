cmake_minimum_required(VERSION 3.5)

project(SpeedBlocks-server)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -Wextra -pedantic")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
add_definitions(-DSFML_NO_DEPRECATED_WARNINGS)

set(EXECUTABLE_NAME SpeedBlocks-server)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h)
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

file(GLOB_RECURSE INCLUDE_DIRS_TMP ${CMAKE_SOURCE_DIR}/src/*)
set(INCLUDE_DIRS)
foreach(f ${INCLUDE_DIRS_TMP})
    get_filename_component(tmp ${f} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${tmp})
endforeach(f)
list(REMOVE_DUPLICATES INCLUDE_DIRS)

target_include_directories(SpeedBlocks-server PUBLIC ${INCLUDE_DIRS})

add_custom_target(run
    COMMAND lxterminal -e ./${EXECUTABLE_NAME}
    DEPENDS SpeedBlocks-server
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

if(POLICY CMP0074)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0074 NEW)
endif()

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc -static -lpthread")
    target_link_libraries(${EXECUTABLE_NAME} iphlpapi.lib)
    set(SFML_STATIC_LIBRARIES TRUE)
endif()

find_package(SFML 2 COMPONENTS network system)
if(NOT SFML_FOUND)
    set(SFML_ROOT "" CACHE PATH "SFML root directory")
    message(FATAL_ERROR "CMake couldn't find SFML. Set the SFML_ROOT entry to SFML's root directory (containing \"include\" and \"lib\" directories).")
endif()
include_directories(${SFML_INCLUDE_DIR})
target_link_libraries(${EXECUTABLE_NAME} ${SFML_LIBRARIES})
if(WIN32)
    target_link_libraries(${EXECUTABLE_NAME} ${SFML_DEPENDENCIES})
endif()

if(POLICY CMP0074)
    cmake_policy(POP)
endif()

find_package(Threads REQUIRED)
target_link_libraries (${EXECUTABLE_NAME} ${CMAKE_THREAD_LIBS_INIT})

if(NOT ${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/Challenges DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()