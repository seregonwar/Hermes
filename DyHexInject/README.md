# DyHexInject - DyMain injector

DyHexInject è un modulo C per l'iniezione e la comunicazione con DyMain.dll. È progettato per essere utilizzato da DyForge come componente di iniezione.

## Caratteristiche

- Iniezione DLL sicura e modulare
- Gestione dei processi target
- Sistema di comunicazione con la DLL iniettata
- Logging di debug integrato
- API C pulita e documentata

## Requisiti

- Windows 7 o superiore
- Visual Studio 2019 o superiore
- Windows SDK 10.0 o superiore

## Compilazione

1. Aprire il prompt dei comandi di Visual Studio
2. Navigare alla directory del progetto
3. Eseguire `build.bat`

## Utilizzo

```c
#include "DyHexInject.h"

// Inizializza DyHexInject
DyHexInjectError error = DyHexInject_Initialize();
if (error != DYHEXINJECT_SUCCESS) {
    // Gestisci l'errore
}

// Apri il processo target
DyHexInjectProcessInfo processInfo;
error = DyHexInject_OpenProcess(processId, &processInfo);
if (error != DYHEXINJECT_SUCCESS) {
    // Gestisci l'errore
}

// Configura l'iniezione
DyHexInjectConfig config = {
    .waitForInjection = true,
    .hideInjection = true,
    .enableDebugLogging = true
};
wcscpy_s(config.dllPath, MAX_PATH, L"path/to/DyMain.dll");

// Inietta la DLL
error = DyHexInject_InjectDll(&processInfo, &config);
if (error != DYHEXINJECT_SUCCESS) {
    // Gestisci l'errore
}

// Avvia la comunicazione
HANDLE communicationHandle;
error = DyHexInject_StartCommunication(&processInfo, &communicationHandle);
if (error != DYHEXINJECT_SUCCESS) {
    // Gestisci l'errore
}

// Invia comandi
error = DyHexInject_SendCommand(communicationHandle, "START_ANALYSIS", strlen("START_ANALYSIS"));
if (error != DYHEXINJECT_SUCCESS) {
    // Gestisci l'errore
}

// Ricevi risposte
char responseBuffer[1024];
size_t bytesReceived;
error = DyHexInject_ReceiveResponse(communicationHandle, responseBuffer, sizeof(responseBuffer), &bytesReceived);
if (error != DYHEXINJECT_SUCCESS) {
    // Gestisci l'errore
}

// Pulizia
DyHexInject_CloseProcess(&processInfo);
DyHexInject_Cleanup();
```

## Integrazione con DyForge

DyHexInject è progettato per essere utilizzato come componente di DyForge. Per integrare:

1. Includi `DyHexInject.h` nel tuo progetto
2. Collega con `DyHexInject.lib`
3. Copia `DyHexInject.dll` nella directory di output

## Note di Sicurezza

- L'iniezione DLL richiede privilegi elevati
- Utilizzare sempre `hideInjection = true` in produzione
- Gestire sempre gli errori e pulire le risorse
- Non esporre l'API direttamente all'utente finale

## Licenza

Questo progetto è rilasciato sotto la licenza MIT.