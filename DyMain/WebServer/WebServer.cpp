#include "WebServer/WebServer.h"
#include <chrono>
#include <sstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <capstone/capstone.h>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace DyMain::WebServer {

// Inizializzazione delle variabili statiche
WebServerManager* WebServerManager::instance = nullptr;
std::mutex WebServerManager::instanceMutex;

WebServerManager& WebServerManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance == nullptr) {
        instance = new WebServerManager();
    }
    return *instance;
}

WebServerManager::~WebServerManager() {
    if (running) {
        stop();
    }
}

bool WebServerManager::start(int port) {
    if (running) return false;
    
    this->port = port;
    mg_mgr_init(&mgr);
    
    // Configura l'URL di ascolto
    std::string listen_url = "http://0.0.0.0:" + std::to_string(port);
    mg_connection* conn = mg_http_listen(&mgr, listen_url.c_str(), eventHandler, this);
    
    if (conn == nullptr) {
        mg_mgr_free(&mgr);
        return false;
    }
    
    // Registra gli endpoint predefiniti
    registerEndpoint("/api/memory", [this](auto conn, auto msg) { handleMemoryEndpoint(conn, msg); });
    registerEndpoint("/api/memory/scan", [this](auto conn, auto msg) { handleMemoryScanEndpoint(conn, msg); });
    registerEndpoint("/api/threads", [this](auto conn, auto msg) { handleThreadsEndpoint(conn, msg); });
    registerEndpoint("/api/thread", [this](auto conn, auto msg) { handleThreadEndpoint(conn, msg); });
    registerEndpoint("/api/modules", [this](auto conn, auto msg) { handleModulesEndpoint(conn, msg); });
    registerEndpoint("/api/hooks", [this](auto conn, auto msg) { handleHooksEndpoint(conn, msg); });
    registerEndpoint("/api/assembly", [this](auto conn, auto msg) { handleAssemblyEndpoint(conn, msg); });
    registerEndpoint("/api/calls", [this](auto conn, auto msg) { handleCallsEndpoint(conn, msg); });
    registerEndpoint("/api/process", [this](auto conn, auto msg) { handleProcessEndpoint(conn, msg); });
    registerEndpoint("/api/command", [this](auto conn, auto msg) { handleCommandEndpoint(conn, msg); });
    
    running = true;
    
    // Avvia il polling loop in un thread separato
    std::thread([this]() {
        while (running) {
            mg_mgr_poll(&mgr, 1000);
            cleanupInactiveConnections();
        }
    }).detach();
    
    return true;
}

void WebServerManager::stop() {
    if (!running) return;
    
    running = false;
    mg_mgr_free(&mgr);
    
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        wsConnections.clear();
    }
}

void WebServerManager::eventHandler(mg_connection* conn, int ev, void* ev_data, void* fn_data) {
    WebServerManager* self = static_cast<WebServerManager*>(fn_data);
    
    switch (ev) {
        case MG_EV_HTTP_MSG: {
            mg_http_message* msg = static_cast<mg_http_message*>(ev_data);
            self->handleHttpRequest(conn, msg);
            break;
        }
        case MG_EV_WS_MSG: {
            mg_ws_message* ws_msg = static_cast<mg_ws_message*>(ev_data);
            self->handleWebSocket(conn, ws_msg);
            break;
        }
        case MG_EV_CLOSE: {
            self->cleanupInactiveConnections();
            break;
        }
    }
}

void WebServerManager::handleHttpRequest(mg_connection* conn, mg_http_message* msg) {
    // Gestisci richiesta di upgrade WebSocket
    if (mg_http_match_uri(msg, "/ws")) {
        mg_ws_upgrade(conn, msg, nullptr);
        std::lock_guard<std::mutex> lock(connectionsMutex);
        wsConnections.push_back(conn);
        return;
    }
    
    // Gestisci richiesta della dashboard
    if (mg_http_match_uri(msg, "/")) {
        std::string dashboardPath = "src/WebServer/dashboard/index.html";
        if (fs::exists(dashboardPath)) {
            mg_http_serve_file(conn, msg, dashboardPath.c_str(), mg_mk_str("text/html"));
        } else {
            mg_http_reply(conn, 404, "Content-Type: text/plain\r\n", "Dashboard non trovata\n");
        }
        return;
    }
    
    // Trova e esegui l'handler per l'endpoint
    std::string uri(msg->uri.ptr, msg->uri.len);
    auto it = endpoints.find(uri);
    if (it != endpoints.end()) {
        it->second(conn, msg);
    } else {
        // 404 Not Found
        mg_http_reply(conn, 404, "Content-Type: application/json\r\n",
                     "{\"error\":\"Endpoint not found\"}\n");
    }
}

void WebServerManager::handleWebSocket(mg_connection* conn, mg_ws_message* ws_msg) {
    // Gestisci messaggi WebSocket (comandi dal client)
    std::string msg(reinterpret_cast<char*>(ws_msg->data.ptr), ws_msg->data.len);
    try {
        json command = json::parse(msg);
        // Processa il comando e invia risposta
        json response = {
            {"status", "success"},
            {"message", "Command received"}
        };
        mg_ws_send(conn, response.dump().c_str(), response.dump().length(), WEBSOCKET_OP_TEXT);
    } catch (const std::exception& e) {
        json error = {
            {"status", "error"},
            {"message", e.what()}
        };
        mg_ws_send(conn, error.dump().c_str(), error.dump().length(), WEBSOCKET_OP_TEXT);
    }
}

void WebServerManager::broadcastEvent(EventType type, const std::string& data) {
    Event event{type, data, std::chrono::system_clock::now().time_since_epoch().count()};
    {
        std::lock_guard<std::mutex> lock(eventQueueMutex);
        eventQueue.push(event);
    }
    
    json eventJson = {
        {"type", static_cast<int>(type)},
        {"data", data},
        {"timestamp", event.timestamp}
    };
    
    broadcastToWebSocket(eventJson.dump());
}

void WebServerManager::broadcastToWebSocket(const std::string& data) {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    for (auto conn : wsConnections) {
        mg_ws_send(conn, data.c_str(), data.length(), WEBSOCKET_OP_TEXT);
    }
}

void WebServerManager::cleanupInactiveConnections() {
    std::lock_guard<std::mutex> lock(connectionsMutex);
    wsConnections.erase(
        std::remove_if(wsConnections.begin(), wsConnections.end(),
                      [](mg_connection* conn) { return conn->is_closing; }),
        wsConnections.end());
}

// Implementazione degli endpoint predefiniti
void WebServerManager::handleMemoryEndpoint(mg_connection* conn, mg_http_message* msg) {
    // Ottieni informazioni sulla memoria
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        json response = {
            {"status", "success"},
            {"data", {
                {"workingSetSize", pmc.WorkingSetSize},
                {"privateUsage", pmc.PrivateUsage},
                {"regions", getMemoryRegions()}
            }}
        };
        mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                     response.dump().c_str());
    } else {
        mg_http_reply(conn, 500, "Content-Type: application/json\r\n",
                     "{\"error\":\"Failed to get memory info\"}\n");
    }
}

void WebServerManager::handleMemoryScanEndpoint(mg_connection* conn, mg_http_message* msg) {
    if (msg->method.len != 4 || strncmp(msg->method.ptr, "POST", 4) != 0) {
        mg_http_reply(conn, 405, "Content-Type: application/json\r\n",
                     "{\"error\":\"Method not allowed\"}\n");
        return;
    }

    try {
        std::string body(msg->body.ptr, msg->body.len);
        json request = json::parse(body);
        std::string pattern = request["pattern"];

        // Esegui la scansione della memoria
        std::vector<uintptr_t> matches = scanMemoryForPattern(pattern);
        
        json response = {
            {"status", "success"},
            {"data", {
                {"matches", matches}
            }}
        };
        mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                     response.dump().c_str());
    } catch (const std::exception& e) {
        mg_http_reply(conn, 400, "Content-Type: application/json\r\n",
                     "{\"error\":\"Invalid request format\"}\n");
    }
}

void WebServerManager::handleThreadsEndpoint(mg_connection* conn, mg_http_message* msg) {
    std::vector<ThreadInfo> threads = getThreadsInfo();
    json response = {
        {"status", "success"},
        {"data", threads}
    };
    mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                 response.dump().c_str());
}

void WebServerManager::handleThreadEndpoint(mg_connection* conn, mg_http_message* msg) {
    // Estrai l'ID del thread dall'URL
    std::string uri(msg->uri.ptr, msg->uri.len);
    size_t pos = uri.find_last_of('/');
    if (pos == std::string::npos) {
        mg_http_reply(conn, 400, "Content-Type: application/json\r\n",
                     "{\"error\":\"Invalid thread ID\"}\n");
        return;
    }

    DWORD threadId = std::stoul(uri.substr(pos + 1));
    ThreadInfo threadInfo = getThreadInfo(threadId);
    
    json response = {
        {"status", "success"},
        {"data", threadInfo}
    };
    mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                 response.dump().c_str());
}

void WebServerManager::handleModulesEndpoint(mg_connection* conn, mg_http_message* msg) {
    std::vector<ModuleInfo> modules = getModulesInfo();
    json response = {
        {"status", "success"},
        {"data", modules}
    };
    mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                 response.dump().c_str());
}

void WebServerManager::handleHooksEndpoint(mg_connection* conn, mg_http_message* msg) {
    std::vector<HookInfo> hooks = getHooksInfo();
    json response = {
        {"status", "success"},
        {"data", hooks}
    };
    mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                 response.dump().c_str());
}

void WebServerManager::handleAssemblyEndpoint(mg_connection* conn, mg_http_message* msg) {
    if (msg->method.len != 4 || strncmp(msg->method.ptr, "POST", 4) != 0) {
        mg_http_reply(conn, 405, "Content-Type: application/json\r\n",
                     "{\"error\":\"Method not allowed\"}\n");
        return;
    }

    try {
        std::string body(msg->body.ptr, msg->body.len);
        json request = json::parse(body);
        uintptr_t address = std::stoull(request["address"].get<std::string>(), nullptr, 16);

        // Disassembla il codice
        std::string assembly = disassembleCode(address);
        
        json response = {
            {"status", "success"},
            {"data", {
                {"assembly", assembly}
            }}
        };
        mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                     response.dump().c_str());
    } catch (const std::exception& e) {
        mg_http_reply(conn, 400, "Content-Type: application/json\r\n",
                     "{\"error\":\"Invalid request format\"}\n");
    }
}

void WebServerManager::handleCallsEndpoint(mg_connection* conn, mg_http_message* msg) {
    std::vector<CallInfo> calls = getCallsInfo();
    json response = {
        {"status", "success"},
        {"data", calls}
    };
    mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                 response.dump().c_str());
}

void WebServerManager::handleProcessEndpoint(mg_connection* conn, mg_http_message* msg) {
    ProcessInfo processInfo = getProcessInfo();
    json response = {
        {"status", "success"},
        {"data", processInfo}
    };
    mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                 response.dump().c_str());
}

void WebServerManager::handleCommandEndpoint(mg_connection* conn, mg_http_message* msg) {
    if (msg->method.len != 4 || strncmp(msg->method.ptr, "POST", 4) != 0) {
        mg_http_reply(conn, 405, "Content-Type: application/json\r\n",
                     "{\"error\":\"Method not allowed\"}\n");
        return;
    }
    
    try {
        std::string body(msg->body.ptr, msg->body.len);
        json command = json::parse(body);
        
        // Processa il comando
        json response = {
            {"status", "success"},
            {"message", "Command executed successfully"}
        };
        mg_http_reply(conn, 200, "Content-Type: application/json\r\n",
                     response.dump().c_str());
    } catch (const std::exception& e) {
        mg_http_reply(conn, 400, "Content-Type: application/json\r\n",
                     "{\"error\":\"Invalid command format\"}\n");
    }
}

// Funzioni helper esposte
bool StartWebServer(int port) {
    return WebServerManager::getInstance().start(port);
}

void StopWebServer() {
    WebServerManager::getInstance().stop();
}

void BroadcastEvent(const std::string& event) {
    WebServerManager::getInstance().broadcastEvent(EventType::PROCESS_INFO, event);
}

// Funzione di utilità per sostituire mg_http_match_uri
bool SimpleHttpMatch(mg_http_message* msg, const char* pattern) {
    if (!msg || !pattern) return false;
    
    std::string uri(msg->uri.ptr, msg->uri.len);
    return uri == pattern;
}

} // namespace DyMain::WebServer 