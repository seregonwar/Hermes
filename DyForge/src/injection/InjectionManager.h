#pragma once

#include "DyHexInject.h"
#include "../../include/CommandTypes.h"
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>

namespace DyForge {
namespace Injection {

class InjectionManager {
public:
    static InjectionManager& getInstance();
    ~InjectionManager();

    bool initialize();
    void cleanup();
    bool injectDll(const std::wstring& dllPath, DWORD processId);
    bool ejectDll(const std::wstring& dllPath, DWORD processId);
    bool startCommunication(DWORD processId);
    void stopCommunication();
    bool sendCommand(const Command& command);
    bool receiveResponse(Response& response);
    bool waitForResponse(Response& response, DWORD timeout);

private:
    InjectionManager();
    static InjectionManager* instance;
    static std::mutex instanceMutex;

    bool initialized;
    DyHexInjectProcessInfo processInfo;
    DyHexInjectCommunication communication;
    std::mutex communicationMutex;
    std::queue<Command> commandQueue;
    std::queue<Response> responseQueue;
    std::condition_variable commandCondition;
    std::thread m_communicationThread;
    std::string lastError;

    void communicationThread();
    bool processCommand(const Command& command);
    void handleError(const char* error);
    void logDebug(const char* format, ...);
    bool initializeSharedMemory();
    void cleanupSharedMemory();
};

} // namespace Injection
} // namespace DyForge 