#include "../../include/DyForge.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>

namespace DyForge {

// Singleton instance
DyForge& DyForge::GetInstance() {
    static DyForge instance;
    return instance;
}

// Constructor
DyForge::DyForge()
    : m_initialized(false)
    , m_processManager(nullptr)
    , m_injectionManager(nullptr)
    , m_analysisManager(nullptr)
    , m_reportingManager(nullptr)
    , m_uiManager(nullptr) {
}

// Destructor
DyForge::~DyForge() {
    Cleanup();
}

// Core functionality
bool DyForge::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    try {
        // Initialize DyHexInject
        DyHexInjectError error = DyHexInject_Initialize();
        if (error != DYHEXINJECT_SUCCESS) {
            throw std::runtime_error("Failed to initialize DyHexInject: " + 
                std::string(DyHexInject_GetErrorString(error)));
        }
        
        // Create managers
        m_processManager = std::make_unique<ProcessManager>();
        m_injectionManager = std::make_unique<InjectionManager>(*m_processManager);
        m_analysisManager = std::make_unique<AnalysisManager>(*m_injectionManager);
        m_reportingManager = std::make_unique<ReportingManager>(*m_analysisManager);
        m_uiManager = std::make_unique<UIManager>(*this);
        
        m_initialized = true;
        return true;
    }
    catch (const std::exception& e) {
        // Log error
        std::ofstream logFile("DyForge_error.log", std::ios::app);
        if (logFile.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            logFile << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
                   << " - Error: " << e.what() << std::endl;
        }
        return false;
    }
}

void DyForge::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    // Cleanup in reverse order
    m_uiManager.reset();
    m_reportingManager.reset();
    m_analysisManager.reset();
    m_injectionManager.reset();
    m_processManager.reset();
    
    // Cleanup DyHexInject
    DyHexInject_Cleanup();
    
    m_initialized = false;
}

bool DyForge::IsInitialized() const {
    return m_initialized;
}

// Process management
bool DyForge::OpenProcess(DWORD processId) {
    if (!m_initialized || !m_processManager) {
        return false;
    }
    return m_processManager->OpenProcess(processId);
}

bool DyForge::CloseProcess() {
    if (!m_initialized || !m_processManager) {
        return false;
    }
    m_processManager->CloseProcess();
    return true;
}

bool DyForge::IsProcessOpen() const {
    return m_processManager && m_processManager->IsProcessOpen();
}

DWORD DyForge::GetCurrentProcessId() const {
    return m_processManager ? m_processManager->GetCurrentProcessId() : 0;
}

// Injection management
bool DyForge::InjectDll(const std::wstring& dllPath) {
    if (!m_initialized || !m_injectionManager) {
        return false;
    }
    return m_injectionManager->InjectDll(dllPath);
}

bool DyForge::UnloadDll() {
    if (!m_initialized || !m_injectionManager) {
        return false;
    }
    return m_injectionManager->UnloadDll();
}

bool DyForge::IsDllInjected() const {
    return m_injectionManager && m_injectionManager->IsDllInjected();
}

// Analysis management
bool DyForge::StartAnalysis() {
    if (!m_initialized || !m_analysisManager) {
        return false;
    }
    return m_analysisManager->StartAnalysis();
}

bool DyForge::StopAnalysis() {
    if (!m_initialized || !m_analysisManager) {
        return false;
    }
    return m_analysisManager->StopAnalysis();
}

bool DyForge::IsAnalysisRunning() const {
    return m_analysisManager && m_analysisManager->IsAnalysisRunning();
}

// Reporting
bool DyForge::GenerateReport(const std::wstring& outputPath) {
    if (!m_initialized || !m_reportingManager) {
        return false;
    }
    return m_reportingManager->GenerateReport(outputPath);
}

bool DyForge::ExportData(const std::wstring& format, const std::wstring& outputPath) {
    if (!m_initialized || !m_reportingManager) {
        return false;
    }
    return m_reportingManager->ExportData(format, outputPath);
}

// Event handling
void DyForge::RegisterEventCallback(const std::string& eventName, EventCallback callback) {
    m_eventCallbacks[eventName].push_back(callback);
}

void DyForge::UnregisterEventCallback(const std::string& eventName) {
    m_eventCallbacks.erase(eventName);
}

// UI management
bool DyForge::ShowMainWindow() {
    if (!m_initialized || !m_uiManager) {
        return false;
    }
    return m_uiManager->ShowMainWindow();
}

bool DyForge::HideMainWindow() {
    if (!m_initialized || !m_uiManager) {
        return false;
    }
    return m_uiManager->HideMainWindow();
}

bool DyForge::IsMainWindowVisible() const {
    return m_uiManager && m_uiManager->IsMainWindowVisible();
}

} // namespace DyForge 