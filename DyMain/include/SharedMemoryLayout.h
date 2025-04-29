#pragma once

#include <windows.h>

#ifdef _WIN32
    #ifdef DYMAIN_EXPORTS
        #define DYMAIN_API __declspec(dllexport)
    #else
        #define DYMAIN_API __declspec(dllimport)
    #endif
#else
    #define DYMAIN_API
#endif

namespace DyMain::SharedMemory {
    // Costanti
    constexpr size_t HEADER_SIZE = 1024;
    constexpr size_t COMMAND_BUFFER_SIZE = 4096;
    constexpr size_t STATE_BUFFER_SIZE = 4096;
    constexpr size_t TOTAL_SIZE = HEADER_SIZE + COMMAND_BUFFER_SIZE + STATE_BUFFER_SIZE;

    // Funzioni di gestione della memoria condivisa
    DYMAIN_API bool CreateSharedMemory();
    DYMAIN_API void CloseSharedMemory();
    
    // Funzioni di gestione dei comandi
    DYMAIN_API DWORD WINAPI ProcessCommands(LPVOID lpParam);
    
    // Funzioni di gestione degli hook
    DYMAIN_API bool InjectHooks();
    DYMAIN_API void RemoveHooks();
} 