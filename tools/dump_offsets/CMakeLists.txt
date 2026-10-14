add_executable(dump_offsets main.cpp)
target_include_directories(dump_offsets PRIVATE ../../core/include)
target_sources(dump_offsets PRIVATE ../../core/src/scanner.cpp)
sc_set_warnings(dump_offsets)
set_target_properties(dump_offsets PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")