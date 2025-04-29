#include "../../include/DyForge.h"
#include <stdexcept>
#include <sstream>

namespace DyForge {

// Constructor
ProcessManager::ProcessManager()
    : m_processId(0)
    , m_processHandle(NULL) {
}

// Destructor
ProcessManager::~ProcessManager() {
    CloseProcess();
}

// Process management
bool ProcessManager::OpenProcess(DWORD processId) {
    if (IsProcessOpen()) {
        CloseProcess();
    }
    
    // Open process using DyHexInject
    DyHexInjectError error = DyHexInject_OpenProcess(processId, &m_processInfo);
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to open process " << processId << ": " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    m_processId = processId;
    m_processHandle = m_processInfo.processHandle;
    return true;
}

void ProcessManager::CloseProcess() {
    if (IsProcessOpen()) {
        DyHexInject_CloseProcess(&m_processInfo);
        m_processId = 0;
        m_processHandle = NULL;
    }
}

bool ProcessManager::IsProcessOpen() const {
    return m_processHandle != NULL;
}

DWORD ProcessManager::GetCurrentProcessId() const {
    return m_processId;
}

} // namespace DyForge 