#pragma once

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

// Version info
#define DYHEXINJECT_VERSION_MAJOR 1
#define DYHEXINJECT_VERSION_MINOR 0

// Error codes
typedef enum {
    DYHEXINJECT_SUCCESS = 0,
    DYHEXINJECT_ERROR_PROCESS_NOT_FOUND = 1,
    DYHEXINJECT_ERROR_ACCESS_DENIED = 2,
    DYHEXINJECT_ERROR_INJECTION_FAILED = 3,
    DYHEXINJECT_ERROR_COMMUNICATION_FAILED = 4,
    DYHEXINJECT_ERROR_INVALID_PARAMETERS = 5,
    DYHEXINJECT_ERROR_EJECTION_FAILED = 6,
    DYHEXINJECT_ERROR_DLL_NOT_FOUND = 7,
    DYHEXINJECT_ERROR_SHARED_MEMORY_FAILED = 8
} DyHexInjectError;

// Process information structure
typedef struct {
    DWORD processId;
    HANDLE processHandle;
    wchar_t processName[MAX_PATH];
    wchar_t processPath[MAX_PATH];
    bool is64Bit;
} DyHexInjectProcessInfo;

// Injection configuration
typedef struct {
    const wchar_t* dllPath;
    bool waitForInjection;
    bool hideInjection;
    bool enableDebugLogging;
    bool enableSharedMemory;
    int sharedMemorySize;
} DyHexInjectConfig;

// Communication structure
typedef struct {
    HANDLE sharedMemoryHandle;
    void* sharedMemoryPtr;
    bool isConnected;
} DyHexInjectCommunication;

// Export macro
#ifdef DYHEXINJECT_EXPORTS
#define DYHEXINJECT_API __declspec(dllexport)
#else
#define DYHEXINJECT_API __declspec(dllimport)
#endif

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif

// Core functions
DYHEXINJECT_API DyHexInjectError DyHexInject_Initialize(void);
DYHEXINJECT_API void DyHexInject_Cleanup(void);

// Process management
DYHEXINJECT_API DyHexInjectError DyHexInject_OpenProcess(DWORD processId, DyHexInjectProcessInfo* processInfo);
DYHEXINJECT_API void DyHexInject_CloseProcess(DyHexInjectProcessInfo* processInfo);

// Injection functions
DYHEXINJECT_API DyHexInjectError DyHexInject_InjectDll(const DyHexInjectProcessInfo* processInfo, const DyHexInjectConfig* config);
DYHEXINJECT_API DyHexInjectError DyHexInject_EjectDll(const DyHexInjectProcessInfo* processInfo, const wchar_t* dllPath);

// Communication functions
DYHEXINJECT_API DyHexInjectError DyHexInject_StartCommunication(const DyHexInjectProcessInfo* processInfo, DyHexInjectCommunication* communication);
DYHEXINJECT_API DyHexInjectError DyHexInject_StopCommunication(DyHexInjectCommunication* communication);
DYHEXINJECT_API DyHexInjectError DyHexInject_SendCommand(DyHexInjectCommunication* communication, const char* command, size_t commandLength);
DYHEXINJECT_API DyHexInjectError DyHexInject_ReceiveResponse(DyHexInjectCommunication* communication, char* buffer, size_t bufferSize, size_t* bytesReceived);

// Utility functions
DYHEXINJECT_API const char* DyHexInject_GetErrorString(DyHexInjectError error);
DYHEXINJECT_API void DyHexInject_EnableDebugLogging(bool enable);
DYHEXINJECT_API bool DyHexInject_IsDllLoaded(const DyHexInjectProcessInfo* processInfo, const wchar_t* dllPath);

// Simplified API
DYHEXINJECT_API int InjectDll(const char* processName, const char* dllPath);
DYHEXINJECT_API int InjectDllByPID(int processId, const char* dllPath);

#ifdef __cplusplus
}
#endif 