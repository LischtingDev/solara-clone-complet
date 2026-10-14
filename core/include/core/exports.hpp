#pragma once
#include <windows.h>
#include <cstdint>

// Appele par l'injecteur apres le mapping de la DLL.
extern "C" __declspec(dllexport) BOOL WINAPI Sc_Init(const wchar_t* pipe_name);
extern "C" __declspec(dllexport) void WINAPI Sc_Shutdown();
extern "C" __declspec(dllexport) uint32_t WINAPI Sc_ProtocolVersion();