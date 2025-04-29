#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <future>
#include <stdexcept>
#include <system_error>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>
#include <algorithm>
#include <numeric>
#include <random>
#include <limits>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdbool>
#include <cstdarg>
#include <cctype>
#include <clocale>
#include <climits>
#include <cfloat>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdalign>
#include <ctgmath>
#include <cuchar>
#include <cwchar>
#include <cwctype>

namespace DyForge {

// Configurazione comunicazione
struct CommunicationConfig {
    // Timeout
    std::chrono::milliseconds commandTimeout{5000};
    std::chrono::milliseconds responseTimeout{5000};
    std::chrono::milliseconds analysisTimeout{30000};
    std::chrono::milliseconds retryDelay{1000};
    
    // Buffer
    size_t commandBufferSize{1024};
    size_t responseBufferSize{1024};
    size_t sharedMemorySize{1024 * 1024}; // 1MB
    
    // Retry
    uint32_t maxRetries{3};
    bool enableRetry{true};
    
    // Logging
    bool enableLogging{true};
    std::string logFile{"dyforge.log"};
    bool logToConsole{true};
};

// Configurazione iniezione
struct InjectionConfig {
    bool waitForInjection = true;
    bool hideInjection = true;
    bool enableDebugLogging = true;
    bool enableSharedMemory = true;
    int sharedMemorySize = 4096;
    std::wstring dllPath;
};

// Tipi di comando
enum class CommandType {
    NONE,
    START_ANALYSIS,
    STOP_ANALYSIS,
    GET_PROCESS_INFO,
    GET_MEMORY_INFO,
    GET_THREAD_INFO,
    GET_MODULE_INFO,
    GET_HOOK_INFO,
    GET_MOD_INFO,
    GET_WEB_SERVER_INFO,
    CUSTOM
};

// Tipi di risposta
enum class ResponseType {
    NONE,
    SUCCESS,
    RESPONSE_ERROR,
    PROCESS_INFO,
    MEMORY_INFO,
    THREAD_INFO,
    MODULE_INFO,
    HOOK_INFO,
    MOD_INFO,
    WEB_SERVER_INFO,
    CUSTOM
};

// Struttura comando
struct Command {
    CommandType type{CommandType::NONE};
    std::vector<uint8_t> data;
    std::chrono::system_clock::time_point timestamp;
    uint32_t id{0};
    bool requiresResponse{true};
};

// Struttura risposta
struct Response {
    ResponseType type{ResponseType::NONE};
    std::vector<uint8_t> data;
    std::chrono::system_clock::time_point timestamp;
    uint32_t commandId{0};
    bool success{false};
    std::string errorMessage;
};

// Configurazione analisi
struct AnalysisConfig {
    // Opzioni di scansione
    bool scanMemory{true};
    bool scanThreads{true};
    bool scanModules{true};
    bool scanHooks{true};
    bool scanMods{true};
    bool scanWebServer{true};
    
    // Configurazione web server
    std::string webServerHost{"localhost"};
    uint16_t webServerPort{8080};
    bool enableWebServer{true};
    
    // Timeout
    std::chrono::milliseconds scanTimeout{30000};
    std::chrono::milliseconds webServerTimeout{5000};
    
    // Logging
    bool enableLogging{true};
    std::string logFile{"analysis.log"};
    bool logToConsole{true};
};

// Configurazione report
struct ReportConfig {
    // Formato
    enum class Format {
        JSON,
        CSV,
        HTML,
        XML
    } format{Format::JSON};
    
    // Opzioni
    bool includeTimestamps{true};
    bool includeStackTraces{true};
    bool includeMemoryDumps{false};
    bool includeThreadInfo{true};
    bool includeModuleInfo{true};
    bool includeHookInfo{true};
    bool includeModInfo{true};
    bool includeWebServerInfo{true};
    
    // Output
    std::string outputFile{"report"};
    bool compressOutput{false};
    std::string compressionFormat{"zip"};
    
    // Logging
    bool enableLogging{true};
    std::string logFile{"report.log"};
    bool logToConsole{true};
};

// Configurazione web server
struct WebServerConfig {
    int port = 8080;
    bool enableWebSocket = true;
    bool enableSSL = false;
    std::string sslCert;
    std::string sslKey;
    std::string rootPath = "web";
};

// Configurazione globale
struct GlobalConfig {
    CommunicationConfig communication;
    InjectionConfig injection;
    AnalysisConfig analysis;
    ReportConfig report;
    WebServerConfig webServer;
    bool enableDebugLogging = true;
    std::string logFile = "DyForge.log";
};

} // namespace DyForge 