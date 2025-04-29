#include "../../include/DyForge.h"
#include <stdexcept>
#include <sstream>

namespace DyForge {

// Constructor
AnalysisManager::AnalysisManager(InjectionManager& injectionManager)
    : m_injectionManager(injectionManager)
    , m_analysisRunning(false) {
}

// Destructor
AnalysisManager::~AnalysisManager() {
    if (m_analysisRunning) {
        StopAnalysis();
    }
}

// Analysis management
bool AnalysisManager::StartAnalysis() {
    if (!m_injectionManager.IsDllInjected()) {
        throw std::runtime_error("No DLL is injected");
    }
    
    if (m_analysisRunning) {
        return true;
    }
    
    // Send start analysis command
    const char* command = "START_ANALYSIS";
    DyHexInjectError error = DyHexInject_SendCommand(
        m_injectionManager.GetCommunication(),
        command,
        strlen(command)
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to send start analysis command: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Wait for response
    char responseBuffer[1024];
    size_t bytesReceived;
    error = DyHexInject_ReceiveResponse(
        m_injectionManager.GetCommunication(),
        responseBuffer,
        sizeof(responseBuffer),
        &bytesReceived
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to receive start analysis response: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Check response
    std::string response(responseBuffer, bytesReceived);
    if (response != "ANALYSIS_STARTED") {
        std::stringstream ss;
        ss << "Unexpected response from DLL: " << response;
        throw std::runtime_error(ss.str());
    }
    
    m_analysisRunning = true;
    return true;
}

bool AnalysisManager::StopAnalysis() {
    if (!m_analysisRunning) {
        return true;
    }
    
    // Send stop analysis command
    const char* command = "STOP_ANALYSIS";
    DyHexInjectError error = DyHexInject_SendCommand(
        m_injectionManager.GetCommunication(),
        command,
        strlen(command)
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to send stop analysis command: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Wait for response
    char responseBuffer[1024];
    size_t bytesReceived;
    error = DyHexInject_ReceiveResponse(
        m_injectionManager.GetCommunication(),
        responseBuffer,
        sizeof(responseBuffer),
        &bytesReceived
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to receive stop analysis response: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Check response
    std::string response(responseBuffer, bytesReceived);
    if (response != "ANALYSIS_STOPPED") {
        std::stringstream ss;
        ss << "Unexpected response from DLL: " << response;
        throw std::runtime_error(ss.str());
    }
    
    m_analysisRunning = false;
    return true;
}

bool AnalysisManager::IsAnalysisRunning() const {
    return m_analysisRunning;
}

} // namespace DyForge 