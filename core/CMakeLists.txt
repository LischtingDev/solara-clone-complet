add_library(sc_core SHARED
    src/dllmain.cpp
    src/log.cpp
    src/scanner.cpp
    src/offsets.cpp
    src/luau_hook.cpp
    src/env.cpp
    src/executor.cpp
    src/dispatcher.cpp)

target_include_directories(sc_core PRIVATE include)
target_link_libraries(sc_core PRIVATE sc_ipc minhook nlohmann_json::nlohmann_json)
sc_set_warnings(sc_core)

target_compile_definitions(sc_core PRIVATE SC_CORE_EXPORTS)
set_target_properties(sc_core PROPERTIES
    OUTPUT_NAME "sc_core"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")