#include "DyHexInject.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <process_id> <dll_path>\n", argv[0]);
        return 1;
    }
    
    // Parse arguments
    DWORD processId = (DWORD)atoi(argv[1]);
    wchar_t dllPath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, argv[2], -1, dllPath, MAX_PATH);
    
    // Initialize DyHexInject
    DyHexInjectError error = DyHexInject_Initialize();
    if (error != DYHEXINJECT_SUCCESS) {
        printf("Failed to initialize DyHexInject: %s\n", DyHexInject_GetErrorString(error));
        return 1;
    }
    
    // Enable debug logging
    DyHexInject_EnableDebugLogging(true);
    
    // Open target process
    DyHexInjectProcessInfo processInfo;
    error = DyHexInject_OpenProcess(processId, &processInfo);
    if (error != DYHEXINJECT_SUCCESS) {
        printf("Failed to open process: %s\n", DyHexInject_GetErrorString(error));
        DyHexInject_Cleanup();
        return 1;
    }
    
    // Configure injection
    DyHexInjectConfig config = {
        .waitForInjection = true,
        .hideInjection = true,
        .enableDebugLogging = true
    };
    wcscpy_s(config.dllPath, MAX_PATH, dllPath);
    
    // Inject DLL
    error = DyHexInject_InjectDll(&processInfo, &config);
    if (error != DYHEXINJECT_SUCCESS) {
        printf("Failed to inject DLL: %s\n", DyHexInject_GetErrorString(error));
        DyHexInject_CloseProcess(&processInfo);
        DyHexInject_Cleanup();
        return 1;
    }
    
    printf("DLL injected successfully!\n");
    
    // Start communication with injected DLL
    HANDLE communicationHandle;
    error = DyHexInject_StartCommunication(&processInfo, &communicationHandle);
    if (error != DYHEXINJECT_SUCCESS) {
        printf("Failed to start communication: %s\n", DyHexInject_GetErrorString(error));
        DyHexInject_CloseProcess(&processInfo);
        DyHexInject_Cleanup();
        return 1;
    }
    
    // Example: Send a command to the injected DLL
    const char* command = "START_ANALYSIS";
    error = DyHexInject_SendCommand(communicationHandle, command, strlen(command));
    if (error != DYHEXINJECT_SUCCESS) {
        printf("Failed to send command: %s\n", DyHexInject_GetErrorString(error));
        DyHexInject_CloseProcess(&processInfo);
        DyHexInject_Cleanup();
        return 1;
    }
    
    // Example: Receive response from the injected DLL
    char responseBuffer[1024];
    size_t bytesReceived;
    error = DyHexInject_ReceiveResponse(communicationHandle, responseBuffer, sizeof(responseBuffer), &bytesReceived);
    if (error != DYHEXINJECT_SUCCESS) {
        printf("Failed to receive response: %s\n", DyHexInject_GetErrorString(error));
        DyHexInject_CloseProcess(&processInfo);
        DyHexInject_Cleanup();
        return 1;
    }
    
    printf("Received response: %.*s\n", (int)bytesReceived, responseBuffer);
    
    // Cleanup
    DyHexInject_CloseProcess(&processInfo);
    DyHexInject_Cleanup();
    
    return 0;
} 