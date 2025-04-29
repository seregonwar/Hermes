#include "InjectionManager.h"
#include "../../../DyHexInject/include/DyHexInjectTypes.h"
#include "../../../DyHexInject/DyHexInject.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <mutex>
#include <cstdarg>
#include <thread>

namespace DyForge {
namespace Injection {

InjectionManager* InjectionManager::instance = nullptr;
std::mutex InjectionManager::instanceMutex;

InjectionManager& InjectionManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance == nullptr) {
        instance = new InjectionManager();
    }
    return *instance;
}

InjectionManager::InjectionManager() : initialized(false) {
}

InjectionManager::~InjectionManager() {
    cleanup();
}

bool InjectionManager::initialize() {
    if (initialized) return true;

    DyHexInjectError error = DyHexInject_Initialize();
    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        return false;
    }

    initialized = true;
    return true;
}

void InjectionManager::cleanup() {
    if (!initialized) return;

    stopCommunication();
    DyHexInject_Cleanup();
    initialized = false;
}

bool InjectionManager::injectDll(const std::wstring& dllPath, DWORD processId) {
    if (!initialized) {
        handleError("InjectionManager not initialized");
        return false;
    }

    DyHexInjectError error = DyHexInject_OpenProcess(processId, &processInfo);
    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        return false;
    }

    DyHexInjectConfig config = {
        dllPath.c_str(),
        true,  // waitForInjection
        false, // hideInjection
        true,  // enableDebugLogging
        true,  // enableSharedMemory
        4096   // sharedMemorySize
    };

    error = DyHexInject_InjectDll(&processInfo, &config);
    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        DyHexInject_CloseProcess(&processInfo);
        return false;
    }

    return true;
}

bool InjectionManager::ejectDll(const std::wstring& dllPath, DWORD processId) {
    if (!initialized) {
        handleError("InjectionManager not initialized");
        return false;
    }

    DyHexInjectError error = DyHexInject_OpenProcess(processId, &processInfo);
    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        return false;
    }

    error = DyHexInject_EjectDll(&processInfo, dllPath.c_str());
    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        DyHexInject_CloseProcess(&processInfo);
        return false;
    }

    DyHexInject_CloseProcess(&processInfo);
    return true;
}

bool InjectionManager::startCommunication(DWORD processId) {
    if (!initialized) {
        handleError("InjectionManager not initialized");
        return false;
    }

    std::lock_guard<std::mutex> lock(communicationMutex);

    DyHexInjectError error = DyHexInject_OpenProcess(processId, &processInfo);
    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        return false;
    }

    error = DyHexInject_StartCommunication(&processInfo, &communication);
    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        DyHexInject_CloseProcess(&processInfo);
        return false;
    }

    m_communicationThread = std::thread(&InjectionManager::communicationThread, this);
    return true;
}

void InjectionManager::stopCommunication() {
    std::lock_guard<std::mutex> lock(communicationMutex);

    if (communication.isConnected) {
        DyHexInject_StopCommunication(&communication);
        DyHexInject_CloseProcess(&processInfo);
    }

    if (m_communicationThread.joinable()) {
        m_communicationThread.join();
    }
}

bool InjectionManager::sendCommand(const Command& command) {
    std::lock_guard<std::mutex> lock(communicationMutex);

    if (!communication.isConnected) {
        handleError("Communication not started");
        return false;
    }

    commandQueue.push(command);
    commandCondition.notify_one();
    return true;
}

bool InjectionManager::receiveResponse(Response& response) {
    std::lock_guard<std::mutex> lock(communicationMutex);

    if (responseQueue.empty()) {
        return false;
    }

    response = responseQueue.front();
    responseQueue.pop();
    return true;
}

bool InjectionManager::waitForResponse(Response& response, DWORD timeout) {
    std::unique_lock<std::mutex> lock(communicationMutex);
    if (commandCondition.wait_for(lock, std::chrono::milliseconds(timeout),
        [this] { return !responseQueue.empty(); })) {
        response = responseQueue.front();
        responseQueue.pop();
        return true;
    }
    return false;
}

void InjectionManager::communicationThread() {
    while (communication.isConnected) {
        Command command;
        {
            std::unique_lock<std::mutex> lock(communicationMutex);
            commandCondition.wait(lock, [this] { 
                return !commandQueue.empty() || !communication.isConnected; 
            });

            if (!communication.isConnected) {
                break;
            }

            command = commandQueue.front();
            commandQueue.pop();
        }

        if (!processCommand(command)) {
            Response errorResponse;
            errorResponse.type = static_cast<ResponseType>(1); // ERROR
            errorResponse.message = lastError;
            
            std::lock_guard<std::mutex> lock(communicationMutex);
            responseQueue.push(errorResponse);
            commandCondition.notify_one();
        }
    }
}

bool InjectionManager::processCommand(const Command& command) {
    std::stringstream ss;
    ss << static_cast<int>(command.type);
    if (!command.parameters.empty()) {
        ss << " " << command.parameters;
    }

    DyHexInjectError error = DyHexInject_SendCommand(
        &communication,
        ss.str().c_str(),
        ss.str().length()
    );

    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        return false;
    }

    char responseBuffer[4096];
    size_t bytesReceived = 0;

    error = DyHexInject_ReceiveResponse(
        &communication,
        responseBuffer,
        sizeof(responseBuffer),
        &bytesReceived
    );

    if (error != DYHEXINJECT_SUCCESS) {
        handleError(DyHexInject_GetErrorString(error));
        return false;
    }

    Response response;
    response.type = ResponseType::SUCCESS;
    response.message = std::string(responseBuffer, bytesReceived);

    std::lock_guard<std::mutex> lock(communicationMutex);
    responseQueue.push(response);
    commandCondition.notify_one();

    return true;
}

void InjectionManager::handleError(const char* error) {
    lastError = error;
    logDebug("Error: %s", error);
}

void InjectionManager::logDebug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);

    std::cout << "[DEBUG] " << buffer << std::endl;
}

bool InjectionManager::initializeSharedMemory() {
    char sharedMemoryName[32];
    sprintf_s(sharedMemoryName, sizeof(sharedMemoryName), "DyForge_%d", processInfo.processId);

    communication.sharedMemory = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        4096,
        sharedMemoryName
    );

    if (!communication.sharedMemory) {
        handleError("Failed to create shared memory");
        return false;
    }

    communication.mappedMemory = MapViewOfFile(
        communication.sharedMemory,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        4096
    );

    if (!communication.mappedMemory) {
        CloseHandle(communication.sharedMemory);
        communication.sharedMemory = NULL;
        handleError("Failed to map shared memory");
        return false;
    }

    return true;
}

void InjectionManager::cleanupSharedMemory() {
    if (communication.mappedMemory) {
        UnmapViewOfFile(communication.mappedMemory);
        communication.mappedMemory = NULL;
    }

    if (communication.sharedMemory) {
        CloseHandle(communication.sharedMemory);
        communication.sharedMemory = NULL;
    }
}

} // namespace Injection
} // namespace DyForge 