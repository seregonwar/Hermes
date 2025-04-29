#ifndef MONGOOSE_CONFIG_H
#define MONGOOSE_CONFIG_H

// Configurazione base di Mongoose
#define MG_ENABLE_LINES 1                  // Abilita il debug con numeri di linea
#define MG_ENABLE_LOG 1                    // Abilita il logging
#define MG_ENABLE_SOCKETPAIR 1             // Necessario per WebSocket
#define MG_ENABLE_MBUF 1                   // Buffer di memoria dinamici
#define MG_ENABLE_HTTP_STREAMING_MULTIPART 1 // Supporto per upload file
#define MG_ENABLE_SSL 1                    // Supporto SSL/TLS
#define MG_ENABLE_IPV6 1                   // Supporto IPv6
#define MG_ENABLE_DIRECTORY_LISTING 0      // Disabilita directory listing per sicurezza

// Configurazioni di sicurezza
#define MG_MAX_HTTP_HEADERS 40             // Limite massimo header HTTP
#define MG_MAX_HTTP_REQUEST_SIZE 8192      // Dimensione massima richiesta
#define MG_MAX_PATH 256                    // Lunghezza massima path
#define MG_MAX_RECV_SIZE (1 * 1024 * 1024) // Buffer ricezione massimo (1MB)

// Configurazioni WebSocket
#define MG_ENABLE_WEBSOCKET 1              // Abilita WebSocket
#define MG_WEBSOCKET_PING_INTERVAL_SECONDS 5 // Intervallo ping WebSocket

// Configurazioni Threading
#define MG_ENABLE_THREADS 1                // Supporto multi-threading
#define MG_THREAD_STACK_SIZE (1024 * 32)   // Stack size per thread

#endif // MONGOOSE_CONFIG_H