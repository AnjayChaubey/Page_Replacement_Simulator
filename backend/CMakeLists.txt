cmake_minimum_required(VERSION 3.15)
project(PageReplacementSimulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Core Algorithm Library
set(CORE_SOURCES
    src/algorithms/FIFO.cpp
    src/algorithms/LRU.cpp
    src/algorithms/Optimal.cpp
    src/algorithms/LFU.cpp
    src/algorithms/SecondChance.cpp
    src/Simulator.cpp
    src/WorkloadGenerator.cpp
)

add_library(page_replacement_core STATIC ${CORE_SOURCES})

# Test Runner Executable
add_executable(test_algorithms tests/test_algorithms.cpp)
target_link_libraries(test_algorithms PRIVATE page_replacement_core)

# Web Server Executable
add_executable(page_replacement_server src/main.cpp)
target_link_libraries(page_replacement_server PRIVATE page_replacement_core)

if(WIN32)
    target_link_libraries(page_replacement_server PRIVATE ws2_32 crypt32)
endif()
