#pragma once

#include "DyMain.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <functional>

namespace DyMain::WebServer {
    /**
     * Avvia il server web sulla porta specificata
     * @param port Porta su cui avviare il server
     * @return true se il server è stato avviato correttamente
     */
    bool StartWebServer(int port);

    /**
     * Ferma il server web
     */
    void StopWebServer();

    void BroadcastEvent(const std::string& event);
} 