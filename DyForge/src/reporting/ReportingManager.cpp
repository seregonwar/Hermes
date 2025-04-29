#include "../../include/DyForge.h"
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <nlohmann/json.hpp>

namespace DyForge {

// Constructor
ReportingManager::ReportingManager(AnalysisManager& analysisManager)
    : m_analysisManager(analysisManager) {
}

// Destructor
ReportingManager::~ReportingManager() {
}

// Reporting
bool ReportingManager::GenerateReport(const std::wstring& outputPath) {
    if (!m_analysisManager.IsAnalysisRunning()) {
        throw std::runtime_error("Analysis is not running");
    }
    
    // Request analysis data
    const char* command = "GET_ANALYSIS_DATA";
    DyHexInjectError error = DyHexInject_SendCommand(
        m_analysisManager.GetInjectionManager().GetCommunication(),
        command,
        strlen(command)
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to send get analysis data command: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Receive analysis data
    char responseBuffer[4096];
    size_t bytesReceived;
    error = DyHexInject_ReceiveResponse(
        m_analysisManager.GetInjectionManager().GetCommunication(),
        responseBuffer,
        sizeof(responseBuffer),
        &bytesReceived
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to receive analysis data: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Parse JSON data
    try {
        nlohmann::json analysisData = nlohmann::json::parse(
            std::string(responseBuffer, bytesReceived)
        );
        
        // Generate report
        std::ofstream reportFile(outputPath);
        if (!reportFile.is_open()) {
            throw std::runtime_error("Failed to open report file");
        }
        
        // Write report header
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        reportFile << "DyForge Analysis Report\n"
                  << "Generated: " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "\n\n";
        
        // Write process information
        const auto& processInfo = m_analysisManager.GetInjectionManager().GetProcessManager().GetProcessInfo();
        reportFile << "Process Information:\n"
                  << "  PID: " << m_analysisManager.GetInjectionManager().GetProcessManager().GetCurrentProcessId() << "\n"
                  << "  Name: " << processInfo.processName << "\n"
                  << "  Path: " << processInfo.processPath << "\n"
                  << "  Architecture: " << (processInfo.is64Bit ? "64-bit" : "32-bit") << "\n\n";
        
        // Write analysis data
        reportFile << "Analysis Results:\n";
        for (const auto& [key, value] : analysisData.items()) {
            reportFile << "  " << key << ": " << value.dump(2) << "\n";
        }
        
        reportFile.close();
        return true;
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to generate report: ") + e.what());
    }
}

bool ReportingManager::ExportData(const std::wstring& format, const std::wstring& outputPath) {
    if (!m_analysisManager.IsAnalysisRunning()) {
        throw std::runtime_error("Analysis is not running");
    }
    
    // Request analysis data
    const char* command = "GET_ANALYSIS_DATA";
    DyHexInjectError error = DyHexInject_SendCommand(
        m_analysisManager.GetInjectionManager().GetCommunication(),
        command,
        strlen(command)
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to send get analysis data command: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Receive analysis data
    char responseBuffer[4096];
    size_t bytesReceived;
    error = DyHexInject_ReceiveResponse(
        m_analysisManager.GetInjectionManager().GetCommunication(),
        responseBuffer,
        sizeof(responseBuffer),
        &bytesReceived
    );
    
    if (error != DYHEXINJECT_SUCCESS) {
        std::stringstream ss;
        ss << "Failed to receive analysis data: " 
           << DyHexInject_GetErrorString(error);
        throw std::runtime_error(ss.str());
    }
    
    // Export data based on format
    if (format == L"json") {
        // Export as JSON
        std::ofstream exportFile(outputPath);
        if (!exportFile.is_open()) {
            throw std::runtime_error("Failed to open export file");
        }
        exportFile << std::string(responseBuffer, bytesReceived);
        exportFile.close();
    }
    else if (format == L"csv") {
        // Export as CSV
        try {
            nlohmann::json analysisData = nlohmann::json::parse(
                std::string(responseBuffer, bytesReceived)
            );
            
            std::ofstream exportFile(outputPath);
            if (!exportFile.is_open()) {
                throw std::runtime_error("Failed to open export file");
            }
            
            // Write CSV header
            for (const auto& [key, value] : analysisData.items()) {
                exportFile << key << ",";
            }
            exportFile << "\n";
            
            // Write CSV data
            for (const auto& [key, value] : analysisData.items()) {
                exportFile << value.dump() << ",";
            }
            exportFile << "\n";
            
            exportFile.close();
        }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string("Failed to export data: ") + e.what());
        }
    }
    else {
        throw std::runtime_error("Unsupported export format: " + std::string(format.begin(), format.end()));
    }
    
    return true;
}

} // namespace DyForge 