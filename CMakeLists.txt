cmake_minimum_required(VERSION 3.24)
project(solara_clone LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SC_BUILD_CORE     "Build injected core DLL" ON)
option(SC_BUILD_INJECTOR "Build injector"          ON)
option(SC_BUILD_TESTS    "Build tests"             ON)
option(SC_BUILD_TOOLS    "Build helper tools"      ON)

include(cmake/CompilerWarnings.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(ipc)

if(SC_BUILD_CORE)
    add_subdirectory(core)
endif()

if(SC_BUILD_INJECTOR)
    add_subdirectory(injector)
endif()

if(SC_BUILD_TOOLS)
    add_subdirectory(tools/dump_offsets)
    add_subdirectory(tools/sigscan_probe)
endif()

if(SC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()