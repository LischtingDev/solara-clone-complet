add_executable(injector
    src/main.cpp
    src/inject_win32.cpp)

target_include_directories(injector PRIVATE include)
target_link_libraries(injector PRIVATE sc_ipc)
sc_set_warnings(injector)

set_target_properties(injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")