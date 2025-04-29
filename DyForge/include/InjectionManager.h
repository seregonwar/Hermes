#pragma once

#include "DyForgeConfig.h"
#include "DyHexInject.h"
#include <memory>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>

namespace DyForge {

class InjectionManager {
public:
    InjectionManager();
    ~InjectionManager();

    // Inizializzazione e pulizia
    bool Initialize();
    void Cleanup();

    // Gestione processo
    bool OpenProcess(DWORD processId);
    void CloseProcess();
    bool IsProcessOpen() const;

    // Iniezione DLL
    bool InjectDyMain(const std::wstring& dllPath);
    bool EjectDyMain();
    bool IsDyMainInjected() const;

    // Comunicazione
    bool StartCommunication();
    void StopCommunication();
    bool SendCommand(const Command& command);
    bool ReceiveResponse(Response& response);
    bool WaitForResponse(Response& response, DWORD timeout);

    // Utility
    const char* GetLastError() const;
    void EnableDebugLogging(bool enable);
    const DyHexInjectProcessInfo& GetProcessInfo() const { return m_processInfo; }

private:
    // Stato interno
    bool m_initialized;
    bool m_processOpen;
    bool m_dllInjected;
    bool m_communicationStarted;
    std::string m_lastError;

    // Gestione processo
    DyHexInjectProcessInfo m_processInfo;
    DyHexInjectConfig m_injectionConfig;
    DyHexInjectCommunication m_communication;

    // Thread di comunicazione
    std::thread m_communicationThread;
    std::mutex m_commandMutex;
    std::condition_variable m_commandCondition;
    std::queue<Command> m_commandQueue;
    std::queue<Response> m_responseQueue;

    // Funzioni private
    void CommunicationThread();
    bool ProcessCommand(const Command& command);
    void HandleError(const char* error);
    void LogDebug(const char* format, ...);
    bool InitializeSharedMemory();
    void CleanupSharedMemory();
};

} // namespace DyForge 