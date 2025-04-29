#pragma once

#include <Windows.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef DYHEXINJECT_EXPORTS
#define DYHEXINJECT_API __declspec(dllexport)
#else
#define DYHEXINJECT_API __declspec(dllimport)
#endif
#else
#define DYHEXINJECT_API
#endif

typedef int DyHexInjectError;

#define DYHEXINJECT_SUCCESS 0
#define DYHEXINJECT_ERROR_PROCESS_NOT_FOUND 1
#define DYHEXINJECT_ERROR_ACCESS_DENIED 2
#define DYHEXINJECT_ERROR_DLL_NOT_FOUND 3
#define DYHEXINJECT_ERROR_INJECTION_FAILED 4
#define DYHEXINJECT_ERROR_COMMUNICATION_FAILED 5
#define DYHEXINJECT_ERROR_INVALID_PARAMETER 6
#define DYHEXINJECT_ERROR_INVALID_PARAMETERS 7
#define DYHEXINJECT_ERROR_EJECTION_FAILED 8
#define DYHEXINJECT_ERROR_SHARED_MEMORY_FAILED 9

typedef struct {
    HANDLE processHandle;
    DWORD processId;
    HMODULE dllHandle;
    void* baseAddress;
    bool is64Bit;
    wchar_t processPath[MAX_PATH];
    wchar_t processName[MAX_PATH];
} DyHexInjectProcessInfo;

typedef struct {
    HANDLE sharedMemory;
    void* mappedMemory;
    bool isConnected;
    size_t memorySize;
    HANDLE sharedMemoryHandle;
    void* sharedMemoryPtr;
} DyHexInjectCommunication;

typedef struct {
    const wchar_t* dllPath;
    bool waitForInjection;
    bool hideInjection;
    bool enableDebugLogging;
    bool enableSharedMemory;
    size_t sharedMemorySize;
} DyHexInjectConfig;

// Function declarations
DYHEXINJECT_API DyHexInjectError DyHexInject_Initialize(void);
DYHEXINJECT_API void DyHexInject_Cleanup(void);
DYHEXINJECT_API DyHexInjectError DyHexInject_OpenProcess(DWORD processId, DyHexInjectProcessInfo* processInfo);
DYHEXINJECT_API void DyHexInject_CloseProcess(DyHexInjectProcessInfo* processInfo);
DYHEXINJECT_API DyHexInjectError DyHexInject_InjectDll(DyHexInjectProcessInfo* processInfo, const DyHexInjectConfig* config);
DYHEXINJECT_API DyHexInjectError DyHexInject_EjectDll(DyHexInjectProcessInfo* processInfo, const wchar_t* dllPath);
DYHEXINJECT_API DyHexInjectError DyHexInject_StartCommunication(DyHexInjectProcessInfo* processInfo, DyHexInjectCommunication* communication);
DYHEXINJECT_API void DyHexInject_StopCommunication(DyHexInjectCommunication* communication);
DYHEXINJECT_API DyHexInjectError DyHexInject_SendCommand(DyHexInjectCommunication* communication, const char* command, size_t length);
DYHEXINJECT_API DyHexInjectError DyHexInject_ReceiveResponse(DyHexInjectCommunication* communication, char* buffer, size_t bufferSize, size_t* bytesReceived);
DYHEXINJECT_API const char* DyHexInject_GetErrorString(DyHexInjectError error);

#ifdef __cplusplus
}
#endif 