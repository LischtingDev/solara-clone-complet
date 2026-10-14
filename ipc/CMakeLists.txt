add_library(sc_ipc STATIC
    src/pipe_win32.cpp)

target_include_directories(sc_ipc PUBLIC include)
sc_set_warnings(sc_ipc)