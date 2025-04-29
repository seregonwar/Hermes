#pragma once

#include "DyForgeConfig.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>

namespace DyForge {

class CommunicationManager {
public:
    CommunicationManager(const CommunicationConfig& config);
    ~CommunicationManager();
    
    // Communication methods
    bool SendCommand(const Command& command);
    bool ReceiveResponse(Response& response);
    bool WaitForResponse(Response& response, std::chrono::milliseconds timeout);
    
    // Analysis methods
    bool StartAnalysis(const AnalysisConfig& config);
    bool StopAnalysis();
    bool GetAnalysisData(std::string& data);
    
    // Process methods
    bool GetProcessInfo(std::string& info);
    bool GetMemoryInfo(std::string& info);
    bool GetThreadInfo(std::string& info);
    bool GetModuleInfo(std::string& info);
    bool GetHookInfo(std::string& info);
    bool GetModInfo(std::string& info);
    bool GetWebServerInfo(std::string& info);
    
    // Custom commands
    bool SendCustomCommand(const std::string& command, std::string& response);
    
private:
    // Internal methods
    void CommunicationThread();
    bool ProcessCommand(const Command& command);
    bool ProcessResponse(const Response& response);
    void HandleTimeout();
    void HandleError(ResponseType errorType, const std::string& errorMessage);
    
    // Configuration
    CommunicationConfig m_config;
    
    // Communication state
    std::atomic<bool> m_running;
    std::thread m_communicationThread;
    std::mutex m_commandMutex;
    std::mutex m_responseMutex;
    std::condition_variable m_commandCondition;
    std::condition_variable m_responseCondition;
    std::queue<Command> m_commandQueue;
    std::queue<Response> m_responseQueue;
    
    // Analysis state
    std::atomic<bool> m_analysisRunning;
    AnalysisConfig m_analysisConfig;
    
    // Error handling
    std::mutex m_errorMutex;
    std::string m_lastError;
    ResponseType m_lastErrorType;
};

} // namespace DyForge 