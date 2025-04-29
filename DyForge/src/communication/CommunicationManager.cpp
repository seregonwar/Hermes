#include "../../include/DyForgeCommunication.h"
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <thread>
#include <json/json.h>

namespace DyForge {

// Constructor
CommunicationManager::CommunicationManager(const CommunicationConfig& config)
    : m_config(config)
    , m_running(false)
    , m_analysisRunning(false) {
}

// Destructor
CommunicationManager::~CommunicationManager() {
    if (m_running) {
        m_running = false;
        m_commandCondition.notify_one();
        if (m_communicationThread.joinable()) {
            m_communicationThread.join();
        }
    }
}

// Communication methods
bool CommunicationManager::SendCommand(const Command& command) {
    if (!m_running) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_commandMutex);
    m_commandQueue.push(command);
    m_commandCondition.notify_one();
    return true;
}

bool CommunicationManager::ReceiveResponse(Response& response) {
    if (!m_running) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_responseMutex);
    if (m_responseQueue.empty()) {
        return false;
    }
    
    response = m_responseQueue.front();
    m_responseQueue.pop();
    return true;
}

bool CommunicationManager::WaitForResponse(Response& response, std::chrono::milliseconds timeout) {
    if (!m_running) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(m_responseMutex);
    if (m_responseCondition.wait_for(lock, timeout, [this] { return !m_responseQueue.empty(); })) {
        response = m_responseQueue.front();
        m_responseQueue.pop();
        return true;
    }
    
    return false;
}

// Analysis methods
bool CommunicationManager::StartAnalysis(const AnalysisConfig& config) {
    if (m_analysisRunning) {
        return false;
    }
    
    m_analysisConfig = config;
    
    Command command;
    command.type = CommandType::START_ANALYSIS;
    command.data = "START_ANALYSIS";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    m_analysisRunning = true;
    return true;
}

bool CommunicationManager::StopAnalysis() {
    if (!m_analysisRunning) {
        return false;
    }
    
    Command command;
    command.type = CommandType::STOP_ANALYSIS;
    command.data = "STOP_ANALYSIS";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    m_analysisRunning = false;
    return true;
}

bool CommunicationManager::GetAnalysisData(std::string& data) {
    if (!m_analysisRunning) {
        return false;
    }
    
    Command command;
    command.type = CommandType::GET_ANALYSIS_DATA;
    command.data = "GET_ANALYSIS_DATA";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.analysisTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    data = response.data;
    return true;
}

// Process methods
bool CommunicationManager::GetProcessInfo(std::string& info) {
    Command command;
    command.type = CommandType::GET_PROCESS_INFO;
    command.data = "GET_PROCESS_INFO";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    info = response.data;
    return true;
}

bool CommunicationManager::GetMemoryInfo(std::string& info) {
    Command command;
    command.type = CommandType::GET_MEMORY_INFO;
    command.data = "GET_MEMORY_INFO";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    info = response.data;
    return true;
}

bool CommunicationManager::GetThreadInfo(std::string& info) {
    Command command;
    command.type = CommandType::GET_THREAD_INFO;
    command.data = "GET_THREAD_INFO";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    info = response.data;
    return true;
}

bool CommunicationManager::GetModuleInfo(std::string& info) {
    Command command;
    command.type = CommandType::GET_MODULE_INFO;
    command.data = "GET_MODULE_INFO";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    info = response.data;
    return true;
}

bool CommunicationManager::GetHookInfo(std::string& info) {
    Command command;
    command.type = CommandType::GET_HOOK_INFO;
    command.data = "GET_HOOK_INFO";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    info = response.data;
    return true;
}

bool CommunicationManager::GetModInfo(std::string& info) {
    Command command;
    command.type = CommandType::GET_MOD_INFO;
    command.data = "GET_MOD_INFO";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    info = response.data;
    return true;
}

bool CommunicationManager::GetWebServerInfo(std::string& info) {
    Command command;
    command.type = CommandType::GET_WEBSERVER_INFO;
    command.data = "GET_WEBSERVER_INFO";
    command.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(command)) {
        return false;
    }
    
    Response response;
    if (!WaitForResponse(response, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (response.type != ResponseType::SUCCESS) {
        HandleError(response.type, response.data);
        return false;
    }
    
    info = response.data;
    return true;
}

// Custom commands
bool CommunicationManager::SendCustomCommand(const std::string& command, std::string& response) {
    Command cmd;
    cmd.type = CommandType::CUSTOM_COMMAND;
    cmd.data = command;
    cmd.timestamp = std::chrono::system_clock::now();
    
    if (!SendCommand(cmd)) {
        return false;
    }
    
    Response resp;
    if (!WaitForResponse(resp, m_config.commandTimeout)) {
        HandleTimeout();
        return false;
    }
    
    if (resp.type != ResponseType::SUCCESS) {
        HandleError(resp.type, resp.data);
        return false;
    }
    
    response = resp.data;
    return true;
}

// Private methods
void CommunicationManager::CommunicationThread() {
    while (m_running) {
        Command command;
        {
            std::unique_lock<std::mutex> lock(m_commandMutex);
            m_commandCondition.wait(lock, [this] { return !m_commandQueue.empty() || !m_running; });
            
            if (!m_running) {
                break;
            }
            
            command = m_commandQueue.front();
            m_commandQueue.pop();
        }
        
        if (!ProcessCommand(command)) {
            HandleError(ResponseType::ERROR, "Failed to process command");
        }
    }
}

bool CommunicationManager::ProcessCommand(const Command& command) {
    std::string response;
    bool success = false;

    switch (command.type) {
        case CommandType::INJECT_DLL:
            success = HandleInjectDLL(command.data, response);
            break;
        case CommandType::EJECT_DLL:
            success = HandleEjectDLL(command.data, response);
            break;
        case CommandType::START_ANALYSIS:
            success = HandleStartAnalysis(command.data, response);
            break;
        case CommandType::STOP_ANALYSIS:
            success = HandleStopAnalysis(command.data, response);
            break;
        case CommandType::START_WEBSERVER:
            success = HandleStartWebServer(command.data, response);
            break;
        case CommandType::STOP_WEBSERVER:
            success = HandleStopWebServer(command.data, response);
            break;
        case CommandType::LOAD_MOD:
            success = HandleLoadMod(command.data, response);
            break;
        case CommandType::UNLOAD_MOD:
            success = HandleUnloadMod(command.data, response);
            break;
        case CommandType::GET_ANALYSIS_RESULTS:
            success = HandleGetAnalysisResults(command.data, response);
            break;
        case CommandType::GET_MOD_INFO:
            success = HandleGetModInfo(command.data, response);
            break;
        case CommandType::GET_HOOK_INFO:
            success = HandleGetHookInfo(command.data, response);
            break;
        case CommandType::GET_WEBSERVER_INFO:
            success = HandleGetWebServerInfo(command.data, response);
            break;
        case CommandType::CUSTOM_COMMAND:
            success = HandleCustomCommand(command.data, response);
            break;
        default:
            response = "Unknown command type";
            success = false;
            break;
    }

    Response resp;
    resp.type = success ? ResponseType::SUCCESS : ResponseType::ERROR;
    resp.data = response;
    resp.timestamp = std::chrono::system_clock::now();

    return ProcessResponse(resp);
}

bool CommunicationManager::ProcessResponse(const Response& response) {
    std::lock_guard<std::mutex> lock(m_responseMutex);
    m_responseQueue.push(response);
    m_responseCondition.notify_one();
    return true;
}

void CommunicationManager::HandleTimeout() {
    HandleError(ResponseType::TIMEOUT, "Command timed out");
}

void CommunicationManager::HandleError(ResponseType errorType, const std::string& errorMessage) {
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastErrorType = errorType;
    m_lastError = errorMessage;
    
    Response response;
    response.type = errorType;
    response.data = errorMessage;
    response.timestamp = std::chrono::system_clock::now();
    
    ProcessResponse(response);
}

bool CommunicationManager::HandleInjectDLL(const std::string& data, std::string& response) {
    try {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(data, root)) {
            response = "Invalid JSON data";
            return false;
        }

        DWORD processId = root["processId"].asUInt();
        if (processId == 0) {
            response = "Invalid process ID";
            return false;
        }

        // Implementa la logica di iniezione della DLL
        // Per ora restituiamo un successo fittizio
        response = "DLL injected successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleEjectDLL(const std::string& data, std::string& response) {
    try {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(data, root)) {
            response = "Invalid JSON data";
            return false;
        }

        DWORD processId = root["processId"].asUInt();
        if (processId == 0) {
            response = "Invalid process ID";
            return false;
        }

        // Implementa la logica di eiezione della DLL
        // Per ora restituiamo un successo fittizio
        response = "DLL ejected successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleStartAnalysis(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di avvio dell'analisi
        // Per ora restituiamo un successo fittizio
        response = "Analysis started successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleStopAnalysis(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di arresto dell'analisi
        // Per ora restituiamo un successo fittizio
        response = "Analysis stopped successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleStartWebServer(const std::string& data, std::string& response) {
    try {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(data, root)) {
            response = "Invalid JSON data";
            return false;
        }

        int port = root["port"].asInt();
        if (port <= 0 || port > 65535) {
            response = "Invalid port number";
            return false;
        }

        // Implementa la logica di avvio del web server
        // Per ora restituiamo un successo fittizio
        response = "Web server started successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleStopWebServer(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di arresto del web server
        // Per ora restituiamo un successo fittizio
        response = "Web server stopped successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleLoadMod(const std::string& data, std::string& response) {
    try {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(data, root)) {
            response = "Invalid JSON data";
            return false;
        }

        std::string modPath = root["path"].asString();
        if (modPath.empty()) {
            response = "Invalid mod path";
            return false;
        }

        // Implementa la logica di caricamento della mod
        // Per ora restituiamo un successo fittizio
        response = "Mod loaded successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleUnloadMod(const std::string& data, std::string& response) {
    try {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(data, root)) {
            response = "Invalid JSON data";
            return false;
        }

        std::string modName = root["name"].asString();
        if (modName.empty()) {
            response = "Invalid mod name";
            return false;
        }

        // Implementa la logica di scaricamento della mod
        // Per ora restituiamo un successo fittizio
        response = "Mod unloaded successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleGetAnalysisResults(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di recupero dei risultati dell'analisi
        // Per ora restituiamo un successo fittizio
        response = "Analysis results retrieved successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleGetModInfo(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di recupero delle informazioni sulle mod
        // Per ora restituiamo un successo fittizio
        response = "Mod info retrieved successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleGetHookInfo(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di recupero delle informazioni sugli hook
        // Per ora restituiamo un successo fittizio
        response = "Hook info retrieved successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleGetWebServerInfo(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di recupero delle informazioni sul web server
        // Per ora restituiamo un successo fittizio
        response = "Web server info retrieved successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

bool CommunicationManager::HandleCustomCommand(const std::string& data, std::string& response) {
    try {
        // Implementa la logica di gestione dei comandi personalizzati
        // Per ora restituiamo un successo fittizio
        response = "Custom command executed successfully";
        return true;
    }
    catch (const std::exception& e) {
        response = std::string("Error: ") + e.what();
        return false;
    }
}

} // namespace DyForge 