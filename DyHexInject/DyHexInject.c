#define DYHEXINJECT_EXPORTS
#include "DyHexInject.h"
#include "include/DyHexInjectTypes.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Debug logging
static bool g_debugLogging = false;
static FILE* g_logFile = NULL;

// Shared memory layout
typedef struct {
    char command[1024];
    char response[1024];
    bool commandReady;
    bool responseReady;
    HANDLE commandEvent;
    HANDLE responseEvent;
} SharedMemoryLayout;

// Helper functions
static void LogDebug(const char* format, ...) {
    if (!g_debugLogging) return;
    
    va_list args;
    va_start(args, format);
    vfprintf(g_logFile, format, args);
    va_end(args);
    fflush(g_logFile);
}

static bool IsProcess64Bit(HANDLE processHandle) {
    BOOL isWow64 = FALSE;
    IsWow64Process(processHandle, &isWow64);
    return !isWow64;
}

// Core functions
DYHEXINJECT_API DyHexInjectError DyHexInject_Initialize(void) {
    if (g_debugLogging) {
        g_logFile = fopen("DyHexInject.log", "a");
        if (!g_logFile) {
            return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
        }
    }
    return DYHEXINJECT_SUCCESS;
}

DYHEXINJECT_API void DyHexInject_Cleanup(void) {
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}

// Process management
DYHEXINJECT_API DyHexInjectError DyHexInject_OpenProcess(DWORD processId, DyHexInjectProcessInfo* processInfo) {
    if (!processInfo) {
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    // Open process with required access rights
    HANDLE processHandle = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | 
        PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE,
        processId
    );

    if (!processHandle) {
        LogDebug("Failed to open process %d: %d\n", processId, GetLastError());
        return DYHEXINJECT_ERROR_PROCESS_NOT_FOUND;
    }

    // Get process information
    processInfo->processId = processId;
    processInfo->processHandle = processHandle;
    processInfo->is64Bit = IsProcess64Bit(processHandle);

    // Get process name and path
    if (!GetModuleFileNameExW(processHandle, NULL, processInfo->processPath, MAX_PATH)) {
        LogDebug("Failed to get process path: %d\n", GetLastError());
        CloseHandle(processHandle);
        return DYHEXINJECT_ERROR_PROCESS_NOT_FOUND;
    }

    wchar_t* lastSlash = wcsrchr(processInfo->processPath, L'\\');
    if (lastSlash) {
        wcscpy_s(processInfo->processName, MAX_PATH, lastSlash + 1);
    } else {
        wcscpy_s(processInfo->processName, MAX_PATH, processInfo->processPath);
    }

    return DYHEXINJECT_SUCCESS;
}

DYHEXINJECT_API void DyHexInject_CloseProcess(DyHexInjectProcessInfo* processInfo) {
    if (processInfo && processInfo->processHandle) {
        CloseHandle(processInfo->processHandle);
        processInfo->processHandle = NULL;
    }
}

// Injection functions
DYHEXINJECT_API DyHexInjectError DyHexInject_InjectDll(DyHexInjectProcessInfo* processInfo, const DyHexInjectConfig* config) {
    if (!processInfo || !config) {
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    LogDebug("Injecting DLL: %ls\n", config->dllPath);

    // Allocate memory for DLL path
    size_t pathSize = (wcslen(config->dllPath) + 1) * sizeof(wchar_t);
    LPVOID remotePath = VirtualAllocEx(
        processInfo->processHandle,
        NULL,
        pathSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!remotePath) {
        LogDebug("Failed to allocate memory: %d\n", GetLastError());
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Write DLL path to remote memory
    if (!WriteProcessMemory(
        processInfo->processHandle,
        remotePath,
        config->dllPath,
        pathSize,
        NULL
    )) {
        LogDebug("Failed to write memory: %d\n", GetLastError());
        VirtualFreeEx(processInfo->processHandle, remotePath, 0, MEM_RELEASE);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Get LoadLibraryW address
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) {
        LogDebug("Failed to get kernel32 handle: %d\n", GetLastError());
        VirtualFreeEx(processInfo->processHandle, remotePath, 0, MEM_RELEASE);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    LPTHREAD_START_ROUTINE loadLibraryAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(
        kernel32,
        "LoadLibraryW"
    );

    if (!loadLibraryAddr) {
        LogDebug("Failed to get LoadLibraryW address: %d\n", GetLastError());
        VirtualFreeEx(processInfo->processHandle, remotePath, 0, MEM_RELEASE);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Create remote thread to load DLL
    HANDLE remoteThread = CreateRemoteThread(
        processInfo->processHandle,
        NULL,
        0,
        loadLibraryAddr,
        remotePath,
        0,
        NULL
    );

    if (!remoteThread) {
        LogDebug("Failed to create remote thread: %d\n", GetLastError());
        VirtualFreeEx(processInfo->processHandle, remotePath, 0, MEM_RELEASE);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Wait for injection if requested
    if (config->waitForInjection) {
        WaitForSingleObject(remoteThread, INFINITE);
    }

    // Cleanup
    CloseHandle(remoteThread);
    VirtualFreeEx(processInfo->processHandle, remotePath, 0, MEM_RELEASE);

    return DYHEXINJECT_SUCCESS;
}

DYHEXINJECT_API DyHexInjectError DyHexInject_EjectDll(DyHexInjectProcessInfo* processInfo, const wchar_t* dllPath) {
    if (!processInfo || !dllPath) {
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    LogDebug("Ejecting DLL: %ls\n", dllPath);

    // Get module handle
    HMODULE moduleHandle = NULL;
    if (!GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        dllPath,
        &moduleHandle
    )) {
        LogDebug("Failed to get module handle: %d\n", GetLastError());
        return DYHEXINJECT_ERROR_EJECTION_FAILED;
    }

    // Get FreeLibrary address
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) {
        LogDebug("Failed to get kernel32 handle: %d\n", GetLastError());
        return DYHEXINJECT_ERROR_EJECTION_FAILED;
    }

    LPTHREAD_START_ROUTINE freeLibraryAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(
        kernel32,
        "FreeLibrary"
    );

    if (!freeLibraryAddr) {
        LogDebug("Failed to get FreeLibrary address: %d\n", GetLastError());
        return DYHEXINJECT_ERROR_EJECTION_FAILED;
    }

    // Create remote thread to unload DLL
    HANDLE remoteThread = CreateRemoteThread(
        processInfo->processHandle,
        NULL,
        0,
        freeLibraryAddr,
        (LPVOID)moduleHandle,
        0,
        NULL
    );

    if (!remoteThread) {
        LogDebug("Failed to create remote thread: %d\n", GetLastError());
        return DYHEXINJECT_ERROR_EJECTION_FAILED;
    }

    // Wait for ejection
    WaitForSingleObject(remoteThread, INFINITE);
    CloseHandle(remoteThread);

    return DYHEXINJECT_SUCCESS;
}

// Communication functions
DYHEXINJECT_API DyHexInjectError DyHexInject_StartCommunication(DyHexInjectProcessInfo* processInfo, DyHexInjectCommunication* communication) {
    if (!processInfo || !communication) {
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    // Create shared memory
    char sharedMemoryName[32];
    sprintf_s(sharedMemoryName, sizeof(sharedMemoryName), "DyHexInject_%d", processInfo->processId);

    communication->sharedMemoryHandle = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(SharedMemoryLayout),
        sharedMemoryName
    );

    if (!communication->sharedMemoryHandle) {
        LogDebug("Failed to create shared memory: %d\n", GetLastError());
        return DYHEXINJECT_ERROR_SHARED_MEMORY_FAILED;
    }

    // Map shared memory
    communication->sharedMemoryPtr = MapViewOfFile(
        communication->sharedMemoryHandle,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedMemoryLayout)
    );

    if (!communication->sharedMemoryPtr) {
        LogDebug("Failed to map shared memory: %d\n", GetLastError());
        CloseHandle(communication->sharedMemoryHandle);
        return DYHEXINJECT_ERROR_SHARED_MEMORY_FAILED;
    }

    // Initialize shared memory
    SharedMemoryLayout* layout = (SharedMemoryLayout*)communication->sharedMemoryPtr;
    layout->commandReady = false;
    layout->responseReady = false;
    layout->commandEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    layout->responseEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    if (!layout->commandEvent || !layout->responseEvent) {
        LogDebug("Failed to create events: %d\n", GetLastError());
        UnmapViewOfFile(communication->sharedMemoryPtr);
        CloseHandle(communication->sharedMemoryHandle);
        return DYHEXINJECT_ERROR_SHARED_MEMORY_FAILED;
    }

    communication->isConnected = true;
    return DYHEXINJECT_SUCCESS;
}

DYHEXINJECT_API void DyHexInject_StopCommunication(DyHexInjectCommunication* communication) {
    if (!communication || !communication->isConnected) {
        return;
    }

    SharedMemoryLayout* layout = (SharedMemoryLayout*)communication->sharedMemoryPtr;
    if (layout) {
        if (layout->commandEvent) CloseHandle(layout->commandEvent);
        if (layout->responseEvent) CloseHandle(layout->responseEvent);
    }

    if (communication->sharedMemoryPtr) {
        UnmapViewOfFile(communication->sharedMemoryPtr);
    }

    if (communication->sharedMemoryHandle) {
        CloseHandle(communication->sharedMemoryHandle);
    }

    communication->isConnected = false;
}

DYHEXINJECT_API DyHexInjectError DyHexInject_SendCommand(DyHexInjectCommunication* communication, const char* command, size_t length) {
    if (!communication || !communication->isConnected || !command) {
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    SharedMemoryLayout* layout = (SharedMemoryLayout*)communication->sharedMemoryPtr;
    if (!layout) {
        return DYHEXINJECT_ERROR_COMMUNICATION_FAILED;
    }

    // Copy command to shared memory
    strncpy_s(layout->command, sizeof(layout->command), command, length);
    layout->commandReady = true;
    SetEvent(layout->commandEvent);

    return DYHEXINJECT_SUCCESS;
}

DYHEXINJECT_API DyHexInjectError DyHexInject_ReceiveResponse(DyHexInjectCommunication* communication, char* buffer, size_t bufferSize, size_t* bytesReceived) {
    if (!communication || !communication->isConnected || !buffer || !bytesReceived) {
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    SharedMemoryLayout* layout = (SharedMemoryLayout*)communication->sharedMemoryPtr;
    if (!layout) {
        return DYHEXINJECT_ERROR_COMMUNICATION_FAILED;
    }

    // Wait for response
    if (WaitForSingleObject(layout->responseEvent, INFINITE) != WAIT_OBJECT_0) {
        return DYHEXINJECT_ERROR_COMMUNICATION_FAILED;
    }

    // Copy response from shared memory
    strncpy_s(buffer, bufferSize, layout->response, sizeof(layout->response));
    *bytesReceived = strlen(buffer);
    layout->responseReady = false;

    return DYHEXINJECT_SUCCESS;
}

// Utility functions
DYHEXINJECT_API const char* DyHexInject_GetErrorString(DyHexInjectError error) {
    switch (error) {
        case DYHEXINJECT_SUCCESS:
            return "Success";
        case DYHEXINJECT_ERROR_PROCESS_NOT_FOUND:
            return "Process not found";
        case DYHEXINJECT_ERROR_ACCESS_DENIED:
            return "Access denied";
        case DYHEXINJECT_ERROR_INJECTION_FAILED:
            return "Injection failed";
        case DYHEXINJECT_ERROR_COMMUNICATION_FAILED:
            return "Communication failed";
        case DYHEXINJECT_ERROR_INVALID_PARAMETERS:
            return "Invalid parameters";
        case DYHEXINJECT_ERROR_EJECTION_FAILED:
            return "Ejection failed";
        case DYHEXINJECT_ERROR_DLL_NOT_FOUND:
            return "DLL not found";
        case DYHEXINJECT_ERROR_SHARED_MEMORY_FAILED:
            return "Shared memory failed";
        default:
            return "Unknown error";
    }
}

void DyHexInject_EnableDebugLogging(bool enable) {
    g_debugLogging = enable;
}

bool DyHexInject_IsDllLoaded(const DyHexInjectProcessInfo* processInfo, const wchar_t* dllPath) {
    if (!processInfo || !dllPath) {
        return false;
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processInfo->processId);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(moduleEntry);

    if (!Module32FirstW(snapshot, &moduleEntry)) {
        CloseHandle(snapshot);
        return false;
    }

    bool found = false;
    do {
        if (_wcsicmp(moduleEntry.szModule, dllPath) == 0) {
            found = true;
            break;
        }
    } while (Module32NextW(snapshot, &moduleEntry));

    CloseHandle(snapshot);
    return found;
}

/*
 * API pubbliche per l'integrazione semplificata
 */

DYHEXINJECT_API int InjectDll(const char* processName, const char* dllPath) {
    if (!processName || !dllPath) {
        printf("Parametri non validi\n");
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    // Trova il processo tramite il nome
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Impossibile creare snapshot dei processi\n");
        return DYHEXINJECT_ERROR_PROCESS_NOT_FOUND;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (_stricmp(processEntry.szExeFile, processName) == 0) {
                processId = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);

    if (processId == 0) {
        printf("Processo '%s' non trovato\n", processName);
        return DYHEXINJECT_ERROR_PROCESS_NOT_FOUND;
    }

    return InjectDllByPID(processId, dllPath);
}

DYHEXINJECT_API int InjectDllByPID(int processId, const char* dllPath) {
    if (processId <= 0 || !dllPath) {
        printf("Parametri non validi\n");
        return DYHEXINJECT_ERROR_INVALID_PARAMETERS;
    }

    // Ottieni l'handle del processo
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        printf("Impossibile aprire il processo (PID: %d)\n", processId);
        return DYHEXINJECT_ERROR_PROCESS_NOT_FOUND;
    }

    // Calcola la dimensione del percorso DLL (in byte)
    SIZE_T dllPathSize = (strlen(dllPath) + 1) * sizeof(char);

    // Alloca memoria nel processo target per il percorso DLL
    LPVOID remoteDllPath = VirtualAllocEx(
        hProcess, 
        NULL, 
        dllPathSize, 
        MEM_COMMIT | MEM_RESERVE, 
        PAGE_READWRITE
    );

    if (!remoteDllPath) {
        printf("Impossibile allocare memoria nel processo target\n");
        CloseHandle(hProcess);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Scrivi il percorso DLL nella memoria del processo target
    if (!WriteProcessMemory(hProcess, remoteDllPath, dllPath, dllPathSize, NULL)) {
        printf("Impossibile scrivere nella memoria del processo target\n");
        VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Ottieni l'indirizzo della funzione LoadLibraryA
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        printf("Impossibile ottenere l'handle di kernel32.dll\n");
        VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    FARPROC pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
    if (!pLoadLibraryA) {
        printf("Impossibile ottenere l'indirizzo di LoadLibraryA\n");
        VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Crea un thread remoto che carica la DLL
    HANDLE hThread = CreateRemoteThread(
        hProcess, 
        NULL, 
        0, 
        (LPTHREAD_START_ROUTINE)pLoadLibraryA, 
        remoteDllPath, 
        0, 
        NULL
    );

    if (!hThread) {
        printf("Impossibile creare thread remoto\n");
        VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    // Attendi che il thread termini
    WaitForSingleObject(hThread, INFINITE);

    // Controlla il risultato dell'iniezione
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    // Pulisci le risorse
    VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    // Se exitCode è 0, l'iniezione è fallita
    if (exitCode == 0) {
        printf("Iniezione fallita\n");
        return DYHEXINJECT_ERROR_INJECTION_FAILED;
    }

    printf("DLL iniettata con successo\n");
    return DYHEXINJECT_SUCCESS;
} 