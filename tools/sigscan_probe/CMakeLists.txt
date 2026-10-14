add_executable(sigscan_probe main.cpp)
target_include_directories(sigscan_probe PRIVATE ../../core/include)
target_sources(sigscan_probe PRIVATE ../../core/src/scanner.cpp)
sc_set_warnings(sigscan_probe)
set_target_properties(sigscan_probe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")