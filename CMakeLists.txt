cmake_minimum_required(VERSION 3.5)

project(SpeedBlocks-server)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -Wextra -pedantic")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
add_definitions(-DSFML_NO_DEPRECATED_WARNINGS)

set(EXECUTABLE_NAME SpeedBlocks-server)
file(GLOB SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h ${CMAKE_SOURCE_DIR}/src/core/*.cpp ${CMAKE_SOURCE_DIR}/src/core/*.h ${CMAKE_SOURCE_DIR}/src/AI/*.cpp ${CMAKE_SOURCE_DIR}/src/AI/*.h ${CMAKE_SOURCE_DIR}/src/utility/*.cpp ${CMAKE_SOURCE_DIR}/src/utility/*.h ${CMAKE_SOURCE_DIR}/src/components/*.cpp ${CMAKE_SOURCE_DIR}/src/components/*.h ${CMAKE_SOURCE_DIR}/src/rooms/*.cpp ${CMAKE_SOURCE_DIR}/src/rooms/*.h )
add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})

target_include_directories(SpeedBlocks-server PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/components ${CMAKE_SOURCE_DIR}/src/rooms ${CMAKE_SOURCE_DIR}/src/utility ${CMAKE_SOURCE_DIR}/src/AI)

add_custom_target(run
    COMMAND lxterminal -e ./${EXECUTABLE_NAME}
    DEPENDS SpeedBlocks-server
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

find_package(SFML 2 COMPONENTS network system)
if(NOT SFML_FOUND)
    set(SFML_ROOT "" CACHE PATH "SFML root directory")
    message(FATAL_ERROR "CMake couldn't find SFML. Set the SFML_ROOT entry to SFML's root directory (containing \"include\" and \"lib\" directories).")
endif()
include_directories(${SFML_INCLUDE_DIR})
target_link_libraries(${EXECUTABLE_NAME} ${SFML_LIBRARIES})

find_package(Threads REQUIRED)
target_link_libraries (${EXECUTABLE_NAME} ${CMAKE_THREAD_LIBS_INIT})

if(NOT ${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_SOURCE_DIR}/Challenges DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()